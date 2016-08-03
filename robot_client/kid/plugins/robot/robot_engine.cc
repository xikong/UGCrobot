/*
 * robot_engine.cc
 *
 *  Created on: 2016年4月7日
 *      Author: Harvey
 */

#include "robot/robot_engine.h"

#include "net/comm_head.h"
#include "core/common.h"
#include "basic/basictypes.h"
#include "basic/template.h"
#include "basic/scoped_ptr.h"
#include "logic/logic_pub_comm.h"
#include "logic/logic_unit.h"

#include "robot/robot_logic.h"

namespace robot_logic {

RobotEngine *RobotEngine::instance_ = NULL;

RobotEngine::RobotEngine()
        : fp_task_log_(NULL) {
    if (!Init()) {
        assert(0);
    }
}

RobotEngine::~RobotEngine() {
    DeinitThreadrw(lock_);

    if (NULL != fp_task_log_) {
        fclose(fp_task_log_);
    }
}

bool RobotEngine::Init() {

    InitThreadrw(&lock_);

    fp_task_log_ = fopen("./task.log", "a+");
    if (NULL == fp_task_log_) {
        LOG_MSG("fopen error task.log");
        return false;
    }

    return true;
}

RobotEngine *RobotEngine::GetInstance() {
    if (NULL == instance_) {
        instance_ = new RobotEngine();
    }
    return instance_;
}

void RobotEngine::FreeInstance() {
    delete instance_;
    instance_ = NULL;
}

bool RobotEngine::PushWaitFeedBackTask(struct TaskHead *task) {

    task->is_feed_back_ = false;
    base_logic::WLockGd lk(lock_);
    wait_feed_back_task_list_.push(task);
}

bool RobotEngine::PushNewTaskInQueue(struct TaskHead *task) {

    base_logic::WLockGd lk(lock_);
    g_ready_task_queue_.push(task);

    LOG_DEBUG2("Add New Task In Queue, task_id = %u, task_type = %d, curr_task_num = %d",
            task->task_id_, task->task_type_, g_ready_task_queue_.size());

    //通知线程去处理
    PostThreadTaskReady();

    return true;
}

bool RobotEngine::PopNewTaskFromQueue(struct TaskHead **task) {

    base_logic::WLockGd lk(lock_);
    *task = g_ready_task_queue_.front();
    g_ready_task_queue_.pop();

    if (NULL == *task) {
        return false;
    }

    LOG_DEBUG2("Pop New Task From Queue, task_id = %u, task_type = %d, curr_task_num = %d",
            (*task)->task_id_, (*task)->task_type_, g_ready_task_queue_.size());

    return true;
}

int32 RobotEngine::GetCurrTaskQueueNum() {

    base_logic::RLockGd lk(lock_);
    int32 curr_task_num = g_ready_task_queue_.size();
    return curr_task_num;
}

bool RobotEngine::MultiRobotTask(const void *msg, int32 len) {

    if (NULL == msg) {
        return false;
    }

    bool r = false;
    struct MultiTaskList multi_task;
    r = multi_task.UnpackStream(msg, len);
    if (!r) {
        LOG_MSG("Receive Server Multi Task List Unpack Failed");
        return false;
    }

    std::list<struct TaskHead*>::iterator iter = multi_task.multi_task_list_
            .begin();
    for (; iter != multi_task.multi_task_list_.end(); ++iter) {
        //添加任务到全局队列中
        PushNewTaskInQueue(*iter);
    }

    return true;
}

bool RobotEngine::PostThreadTaskReady() {

    struct server *srv = logic::CoreSoUtils::GetSRV();
    if (NULL == srv) {
        LOG_MSG("PostThreadTaskReady GetSRV Failed");
        return false;
    }

    if (NULL == srv->user_addtask) {
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    int ret = 0;
    struct plugin *robot_pl = RobotLogic::GetInstance()->GetRobotPlugin();
    ret = srv->user_addtask(srv, TASK_READY, robot_pl);
    if (0 != ret) {
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    return true;
}

bool RobotEngine::OnTaskThreadFunc(struct server *srv, int fd, void* data) {

    switch (fd) {
        case TASK_READY: {
            StartTaskWork();
            break;
        }
        default:
            break;
    }

    return true;
}

void RobotEngine::StartTaskWork() {

    bool r = false;

    //获取一个任务
    struct TaskHead *task = NULL;
    PopNewTaskFromQueue(&task);
    if (NULL == task) {
        LOG_MSG("PopNewTaskFromQueue Failed");
        return;
    }

    //获得任务处理类
    base_logic::TaskEngine *engine = GetTaskEngineByType(task->task_type_);
    if (NULL == engine) {
        LOG_MSG2("Not Find TaskType = %d TaskEngine", task->task_type_);
        return;
    }

    //执行任务
    string str_response;
    engine->StartTaskWork(task, str_response);

    //任务结果写人日志文件
    WriteLogFile(task, str_response);

    //向服务器反馈任务的状态
    r = FeedBackTaskStatus(task);
    if (!r) {
        PushWaitFeedBackTask(task);
    } else {
        delete task;
        task = NULL;
    }

    LOG_MSG("TaskWork Job finsh");

    return;
}

void RobotEngine::CheckIsHaveFeedBackTask() {

    base_logic::WLockGd lk(lock_);

    bool r = false;
    while (wait_feed_back_task_list_.size() > 0) {
        struct TaskHead *task = wait_feed_back_task_list_.front();
        r = FeedBackTaskStatus(task);
        if (r) {
            wait_feed_back_task_list_.pop();
        }
    }
}

bool RobotEngine::FeedBackTaskStatus(struct TaskHead *task) {

    bool r = false;

    struct FeedBackTaskStatus feedback_status_msg;
    MAKE_HEAD(feedback_status_msg, C2S_FEEDBACK_TASK_STATUS, 0, 0, 0, 0);
    feedback_status_msg.is_success = task->is_success_;

    feedback_status_msg.error_code = task->error_no_;
    feedback_status_msg.server_id_ = task->feed_server_id_;
    feedback_status_msg.crawler_id_ = RobotLogic::GetInstance()->GetRobotId();
    feedback_status_msg.crawler_type_ = robot_type;

    feedback_status_msg.task_type = task->task_type_;
    feedback_status_msg.task_id = task->task_id_;
    feedback_status_msg.cookie_id = task->cookie_id_;

    r = RobotLogic::GetInstance()->SendMsgToRouter(&feedback_status_msg);
    if (!r) {
        LOG_MSG("SendRouterMsg Failed");
        return false;
    }

    LOG_DEBUG2("Feedback ServerId = %d, task_id = %d, task_type = %d, cookie_id = %d, is_success = %d, error_msg = %s\n",
            task->feed_server_id_,
            task->task_id_,
            task->task_type_,
            task->cookie_id_,
            feedback_status_msg.is_success,
            feedback_status_msg.error_code.c_str());

    return true;
}

bool RobotEngine::WriteLogFile(struct TaskHead *task, const string &response) {

    base_logic::WLockGd lk(lock_);

    //将日志文件加锁
    base_logic::WFileLockGd lkf(fp_task_log_);

    //组合日志
    std::stringstream os;
    os << logic::SomeUtils::GetLocalTime(time(NULL)) << "\n";
    os << "task_id = " << task->task_id_ << ", ";
    os << "task_type = " << task->task_type_ << ", ";
    os << "cookie_id = " << task->cookie_id_ << ", ";
    os << "user_id = " << task->user_id_ << ", ";
    os << "is_success = " << (int) task->is_success_ << ",\n";
    os << "task_referer = " << task->pre_url_ << ",\n";
    os << "task_content = " << task->content_ << ",\n";
    os << "response = " << response << "\n\n";

    //将任务信息写到日志中
    string str_log = os.str();
    fseek(fp_task_log_, 0, SEEK_END);
    if (fwrite(str_log.c_str(), str_log.size(), 1, fp_task_log_) < 0) {
        LOG_MSG("fwrite response error");
    }
    fflush(fp_task_log_);
    os.str("");

    return true;
}

void RobotEngine::Test() {

//淘股吧
#if 0
    struct TaskTaoGuBaPacket *task = new struct TaskTaoGuBaPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_TAOGUBA;

    task->forge_ip_ = "182.90.4.74:80";
    task->forge_ua_ = "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.7) Gecko/20071013 Firefox/2.0.0.7 Flock/0.9.1.3";
    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析 baidu。哈哈nu 杠 w1x";
    task->topicID_ = "1480253";
    task->subject_ = "怕个卵啊！上！";
    task->cookie_ = "JSESSIONID=911A19A2CB703E6254067DDBA772D303-n1; tgbpwd=834B80EA03Dcsmk3wbwjv7vkgv; tgbuser=1706049; ";
    PushNewTaskInQueue(task);

#endif

//贴吧
#if 0
    struct TaskTieBaPacket *task = new struct TaskTieBaPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_TIEBA;

    task->pre_url_ = "http://tieba.baidu.com/p/3897491915";
    task->kw_ = "sh600068股票 ";
    task->fid_ = "1508953";
    task->repost_id_ = "71632469505";
    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析 baidu.哈哈nu 杠 w1x";

    task->cookie_ = "tbs=3c55f2a7f20cd3471464763985;user_id=2219381998;BAIDUID=69CF26B02F3F5A5B3754F76C2109DA29:FG=1; BDUSS=EtjfkdxdXQwdS1tTzVmRkhYVHlyUnpMUDU1ZXBNbFVwc09hbjJXVklwSlFFM1pYQVFBQUFBJCQAAAAAAAAAAAEAAADuFEmEa29iZWJyeWFudDk2OTgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFCGTldQhk5XT; BIDUPSID=69CF26B02F3F5A5B3754F76C2109DA29; H_PS_PSSID=19291_20144_18286_1466_18280_17948_19568_18560_15077_12322; PSTM=1464763979; HISTORY=3f29f9a203de40e09cdd47bd16dfe1bcd71c56; HOSUPPORT=1; PTOKEN=960e970e51b7edfa3e95adc82db7a09a; SAVEUSERID=eafaeb32be21823c0a67a185fbdd5d; STOKEN=2943cfdef654c04cf1861688007810cc878293057b1a708019c11d671c1b08cf; UBI=fi_PncwhpxZ%7ETaKAZqH2c%7ECthSfnPOrKaPR8zjNRli%7E4DEpqt3YicnrxN%7Ee1gsE3vlQivwNH4GCZ7Lp3lPeQoNa7jusf%7Eo7K5X-2v5fQ4o5MBllMYoWTiz%7ESR-2ZeUGq8f--EPa6pHq6uH5ccBzH8ZD%7EXMXPQ__; USERNAMETYPE=3; TIEBAUID=8f42a9432ef22d8914b88e6f; TIEBA_USERTYPE=217c8d3e10645a62d3a93613; BDSVRTM=0; BD_HOME=0; ";

    PushNewTaskInQueue(task);

#endif

//雪球
#if 0

    struct TaskXueQiuPacket *task = new struct TaskXueQiuPacket;
    task->task_id_ = 1;
    task->task_type_ = TASK_XUEQIU;
    task->pre_url_ = "https://xueqiu.com/6654628252/69410718";
    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析  http://baidu.nu/w1x";

    task->cookie_ = "bid=f66234ba07499c9262086cc93b84f2d3_iowq4vy8; s=fod11clcub; u=2767457866; xq_a_token=88d452bad78afa752e5ae69a88455bc1097999ee; xq_is_login=1; xq_r_token=4003e69cf22e057158a1eadc18b4994db1265cc7; xq_token_expire=Sun%20Jun%2026%202016%2018%3A22%3A25%20GMT%2B0800%20(CST); xqat=88d452bad78afa752e5ae69a88455bc1097999ee; ";


    PushNewTaskInQueue(task);

#endif

//东方股吧
#if 0

    struct TaskIGuBaPacket *task = new struct TaskIGuBaPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_IGUBA;

    task->pre_url_ = "http://money.eastmoney.com/news/1282,20160517624551717.html";
    task->content_ = "学习了";
    task->cookie_ = "st_pvi=47292841728579; st_si=01590777518671; HAList=a-sz-300059-%u4E1C%u65B9%u8D22%u5BCC; em_hq_fls=old; VerifyCode=key=183007415410&gps=222.73.55.92&validate=DE9D53F39158C003; ut=FobyicMgeV5n3saZh_euZ6ZGJttwXu2bz277zHB-uYO9A882ZZi1D-Llx4-piklvHW7gXcr2Pa1lwTA44xHb9XRA-wdAQ0KnDYqQZMm2zk9EG9I8r4X0wSrDGwjJCZIenqyupJstFEdMGHvs9Rk4enNZQ-pubToPnEkJ-x9uFw6vbZt7YvzcwOT5_2qIolBft9VLSVQ60aBqPPiG6WB7e1FcABgBw6IGtcJAfLjZGV6HJvt94TVpyx5el2szOa2kX4GDtf748RnYTFRjqLTVDofC2FThk2be; ct=XHv8_KD9IYG7_DOTOjyRjckHfsAtyeZSaSn6n9kuSEKMnGHvaebhPrIhvTsrQ_sGC1AZFsindLFHAdD6OgHY8dv3HqZ94mjaAIVZTIP4Aiu0InxmlPcAWTElKLPdhdqctKMbrZ-oncED0B34U-2OmeV-QHGcmxWmLTLPr9lFTI8; uidal=1272044633894944%e5%9c%9f%e8%b1%86%e6%98%af%e9%81%93%e8%8f%9c; pi=1272044633894944%3ba1272044633894944%3b%e5%9c%9f%e8%b1%86%e6%98%af%e9%81%93%e8%8f%9c%3byqEGLL1wdh%2bcstZeXlvQi0GxopqCGgqLliiC6iGfU9bjSobrQP77OAsg0IAtW40id6N3onwEo2xRLvV1ROJiKXhLkAxf8mXFBEft1LzSzqNLjbgW%2f9pAYQyCB4M6vEZPI0oxlAPSwcDYcXdf%2bXYx58MZUZD%2b0fEufveRYLBeXE2XLFUGAoA2zX3gD1Tdoyt9v8fmKWre%3bYGnwQlO0erqehy3Fp0uUibkSSj%2bCOlJ5910N5h7pBD8QWKlQIN%2bQasB1iUmuJVvJOH13pZ86l3THLVqOatGkHSBUKC6RYotB%2be%2bc5VibtO6%2br232Csd%2bU9wGx%2f210e5hRiNmTzQ6VHax7gHn4NHbbBAscqBECQ%3d%3d; pu=18621532630";

    PushNewTaskInQueue(task);

#endif

//微博
#if 0

    struct TaskWeiBoPacket *task = new struct TaskWeiBoPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_WEIBO;

    task->cookie_ = "SINAGLOBAL=6901328550210.859.1462757427650; wb_bub_hot_5722596182=1; _s_tentry=-; Apache=1872796432905.1328.1464578287927; ULV=1464578287933:10:10:1:1872796432905.1328.1464578287927:1464329329675; appkey=; WBStore=8ca40a3ef06ad7b2|undefined; WBtopGlobal_register_version=f81ab92b992b2688; UOR=www.liaoxuefeng.com,widget.weibo.com,login.sina.com.cn; myuid=5766620920; un=rksodpdkh@sina.cn; wvr=6; SUE=es%3D1fa9ba93efe969bb9bb6aee80ab5753e%26ev%3Dv1%26es2%3D4477c7cd1fc1c0104691758688acecec%26rs0%3DO0zNPGAiWJxH%252F7ru58p2C0GBM9oN5Y4GcOCVQR%252Fgm1kx595rxaAPzWOg1bWQLO4AkLxmWPrvI1nC1JEgBqoQQxfKxX7L9Mlb%252BfbptPtGfJeJ%252B%252B3lsLDc6TPDmatyh5FagxS8ZnxAZYkN5u5WE8uvPav53UKOZmYwZLZvEa8d2ms%253D%26rv%3D0; SRF=1464743790; SUB=_2A256Skc-DeTxGeNJ7VQX8i7FyTyIHXVZPj_2rDV8PUNbuNBeLUbekW9LHetnZNcgc_nlIAajRKiimoO-pTe13w..; SUBP=0033WrSXqPxfM725Ws9jqgMF55529P9D9W5rh5DdBQbI0WArR2PkiCLF5JpX5K2hUgL.Fo-NSoqceo54eo52dJLoIf7LxK-L12qLB.eLxKMLB-zLBK-LxKqLBK5L12zLxK-LBK-LBo.LxK.L1-2L1K5LxKML1--L1KzLxKBLB.2L12zLxK-LB--L1--LxK-LBKqL1hzLxKqLB-BLBKBRSntt; YF-Ugrow-G0=3a02f95fa8b3c9dc73c74bc9f2ca4fc6; ALF=1465348590; ALC=ac%3D0%26bt%3D1464743790%26cv%3D5.0%26et%3D1496279790%26uid%3D5766620920%26vf%3D0%26vs%3D0%26vt%3D0%26es%3D03983666292b507bacb01f038d793265; sso_info=v02m6alo5qztKWRk5SljoSYpZCkjKWRk5ylkJSIpZCTlKWRk5ilkJOApY6EiKWRk5SlkJOUpZCjpKWRk6SljpSEpY6TgKadlqWkj5OUt42jmLaMo4C5jKOAwA==; tgc=TGT-NTc2NjYyMDkyMA==-1464743789-gz-D7A1AF2914180F5206CB1CB26A747FD9; LT=1464743790; YF-V5-G0=c072c6ac12a0526ff9af4f0716396363; SUS=SID-5766620920-1464743790-GZ-x0guk-b8164756d2c32b09560d5c0633322d93; SRT=E.vAfsiqR-iZznKOznvcS1LXmBvXvCvXMQOZWlvnEABvzvvv4mk1rklXRmvvvAwnVmPr_zxXfCvvAivOmKvAmLvAmMvXvCFvmvFXMQOZWl*B.vAflW-P9Rc0lR-yk-DvnJqiQVbiRVPBtS!r3JZPQVqbgVdWiMZ4siOzu4DbmKPVsVcXsMFMC4dHHJEWBVbzp4Fk9AroFi49ndDPIJcYPSrnlMc0kObi6VdibUq4rSd0sJcM1OFyHSZWJ5mjkODmpV4oCIcPJ5mkiOmzk5!oCNpsJ5mkoOmHIi4noJe9J5mjkOmHII4oCUpuJ5mjlODmkJ!noNqPJ5mkiODmkI4noN39J5mkCOmzlJ!noJe9J5mjkOmzkI4noINsJ5mjkOmHIV4noTG9J5mkoOmzkA!oCNrHN4-urWv77; SUP=cv%3D1%26bt%3D1464743790%26et%3D1464830190%26d%3Dc909%26i%3D2d93%26us%3D1%26vf%3D0%26vt%3D0%26ac%3D0%26st%3D0%26uid%3D5766620920%26name%3Drksodpdkh%2540sina.cn%26nick%3D%25E5%258F%25BC%25E7%25AB%25A5%25E6%25A0%258B%25E5%25A5%25B9%25E9%259A%2590%26fmp%3D%26lcp%3D; SSOLoginState=1464743790; SUHB=0id7T1oEt5dAzv; ";

    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析  http://baidu.去掉汉字nu/w1x";

    task->topic_id_ = "3981116925265076";
    task->host_uin_ = "5139735642";

    PushNewTaskInQueue(task);

#endif

#if 0

    struct TaskTongHuaShunPacket *task = new TaskTongHuaShunPacket;
    task->task_id_ = 1;
    task->task_type_ = TASK_TONGHUASHUN;

    task->cookie_ = "uname=土豆是道菜;avatar=http://space.10jqka.com.cn/avatar/6897/339276897.gif;user=MDrNwba5yse1wLLLOjpOb25lOjUwMDozNDkyNzY4OTc6NywxMTExMTExMTExMSw0MDs0NCwxMSw0MDs2LDEsNDA7NSwxLDQwOjI3Ojo6MzM5Mjc2ODk3OjE0NjUxNzUxODE6OjoxNDY1MTc1MTAwOjYwNDgwMDowOmFmMmUwNWU1NzQ0Yzc3YWZiZjM0ODRjODE1YmE4MTBkOmRlZmF1bHRfMg%3D%3D; userid=339276897; u_name=%CD%C1%B6%B9%CA%C7%B5%C0%B2%CB; escapename=%25u571f%25u8c46%25u662f%25u9053%25u83dc; ticket=8adfb0250f3a9fc11e7992bb6d30d79c; Hm_lvt_f79b64788a4e377c608617fba4c736e2=1464945435,1465175063; Hm_lpvt_f79b64788a4e377c608617fba4c736e2=1465175247; Hm_lvt_da7579fd91e2c6fa5aeb9d1620a9b333=1465175856,1465175862,1465175916; Hm_lpvt_da7579fd91e2c6fa5aeb9d1620a9b333=1465175916; Hm_lvt_78c58f01938e4d85eaf619eae71b4ed1=1464945435,1465175063; Hm_lpvt_78c58f01938e4d85eaf619eae71b4ed1=1465178585";

    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析 baidu去掉中文。nu/2jv9";
    task->pre_url_ = "http://stock.10jqka.com.cn/20160606/c590784409.shtml";

    PushNewTaskInQueue(task);

#endif

}

} /* namespace robot_logic */
