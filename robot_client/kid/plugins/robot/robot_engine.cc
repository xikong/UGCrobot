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
    if(!Init()){
        assert(0);
    }
}

RobotEngine::~RobotEngine() {
    DeinitThreadrw(lock_);

    if(NULL != fp_task_log_){
        fclose(fp_task_log_);
    }
}

bool RobotEngine::Init(){

    InitThreadrw(&lock_);

    fp_task_log_ = fopen("./task.log", "a+");
    if(NULL == fp_task_log_){
        LOG_MSG("fopen error task.log");
        return false;
    }

    return true;
}

RobotEngine *RobotEngine::GetInstance(){
    if(NULL == instance_){
        instance_ = new RobotEngine();
    }
    return instance_;
}

void RobotEngine::FreeInstance(){
    delete instance_;
    instance_ = NULL;
}

bool RobotEngine::PushNewTaskInQueue(struct TaskHead *task){

    base_logic::WLockGd lk(lock_);
    g_ready_task_queue_.push(task);

    LOG_DEBUG2("Add New Task In Queue, task_id = %u, task_type = %d, curr_task_num = %d",
            task->task_id_, task->task_type_, g_ready_task_queue_.size());

    //通知线程去处理
    PostThreadTaskReady();

    return true;
}

bool RobotEngine::PopNewTaskFromQueue(struct TaskHead **task){

    base_logic::WLockGd lk(lock_);
    *task = g_ready_task_queue_.front();
    g_ready_task_queue_.pop();

    if(NULL == *task){
        return false;
    }

    LOG_DEBUG2("Pop New Task From Queue, task_id = %u, task_type = %d, curr_task_num = %d",
            (*task)->task_id_, (*task)->task_type_, g_ready_task_queue_.size());

    return true;
}

int32 RobotEngine::GetCurrTaskQueueNum(){

    base_logic::RLockGd lk(lock_);
    int32 curr_task_num = g_ready_task_queue_.size();
    return curr_task_num;
}

bool RobotEngine::MultiRobotTask(const void *msg, int32 len){

    if(NULL == msg){
        return false;
    }

    bool r = false;
    struct MultiTaskList multi_task;
    r = multi_task.UnpackStream(msg, len);
    if(!r){
        LOG_MSG("Receive Server Multi Task List Unpack Failed");
        return false;
    }

    std::list<struct TaskHead*>::iterator iter = multi_task.multi_task_list_.begin();
    for(; iter != multi_task.multi_task_list_.end(); ++iter){
        //添加任务到全局队列中
        PushNewTaskInQueue(*iter);
    }

    return true;
}

bool RobotEngine::PostThreadTaskReady(){

    struct server *srv = logic::CoreSoUtils::GetSRV();
    if(NULL == srv){
        LOG_MSG("PostThreadTaskReady GetSRV Failed");
        return false;
    }

    if(NULL == srv->user_addtask){
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    int ret = 0;
    struct plugin *robot_pl = RobotLogic::GetInstance()->GetRobotPlugin();
    ret = srv->user_addtask(srv, TASK_READY, robot_pl);
    if(0 != ret){
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    return true;
}

bool RobotEngine::OnTaskThreadFunc(struct server *srv, int fd, void* data){

    switch(fd){
    case TASK_READY:{
        StartTaskWork();
        break;
    }
    default:
        break;
    }

    return true;
}

void RobotEngine::StartTaskWork(){

    LOG_MSG("Start Task WorkJob");

    bool r = false;

    //获取一个任务
    struct TaskHead *task = NULL;
    PopNewTaskFromQueue(&task);
    if(NULL == task){
        LOG_MSG("PopNewTaskFromQueue Failed");
        return;
    }

    //将task放入智能指针
    scoped_ptr<struct TaskHead> pTask(task);

    //获得任务处理类
    base_logic::TaskEngine *engine = GetTaskEngineByType(task->task_type_);
    if(NULL == engine){
        LOG_MSG2("Not Find TaskType = %d TaskEngine", task->task_type_);
        return;
    }

    //执行任务
    string str_response;
    r = engine->StartTaskWork(task, str_response);

    //向服务器反馈任务的状态
    FeedBackTaskStatus(task, r);

    //任务结果写人日志文件
    WriteLogFile(task, str_response);

    LOG_MSG("TaskWork Job finsh");

    return;
}

bool RobotEngine::FeedBackTaskStatus(struct TaskHead *task, bool is_success){

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
    if(!r){
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

bool RobotEngine::WriteLogFile(struct TaskHead *task, const string &response){

    //将日志文件加锁
    base_logic::WFileLockGd lk(fp_task_log_);

    //组合日志
    std::stringstream os;
    os << logic::SomeUtils::GetLocalTime(time(NULL)) << "\n";
    os << "task_id = " << task->task_id_ << ", ";
    os << "task_type = " << task->task_type_ << ", ";
    os << "cookie_id = " << task->cookie_id_ << ", ";
    os << "user_id = " << task->user_id_ << ", ";
    os << "is_success = " << (int)task->is_success_ << ",\n";
    os << "task_referer = " << task->pre_url_ << ",\n";
    os << "task_content = " << task->content_ << ",\n";
    os << "response = " << response << "\n\n";

    //将任务信息写到日志中
    string str_log = os.str();
    fseek(fp_task_log_, 0, SEEK_END);
    if ( fwrite (str_log.c_str(), str_log.size(), 1, fp_task_log_) < 0) {
        LOG_MSG("fwrite response error");
    }

    fflush(fp_task_log_);

    LOG_DEBUG2("task_detail = %s", str_log.c_str());

    return true;
}

void RobotEngine::Test(){

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

    task->pre_url_ = "http://tieba.baidu.com/p/4542573958";
    task->kw_ = "美的";
    task->fid_ = "125819";
    task->content_ = "[img+pic_type=0+width=560+height=285]http://imgsrc.baidu.com/forum/pic/item/94e02e08c93d70cf2916ecefffdcd100b8a12b47.jpg[/img]";
    task->cookie_ = "tbs=8b7680f2a7f5ff061463052789;user_id=2219290345;BAIDUID=690F68B321216CAD38673E191C509FE9:FG=1; BDUSS=mZSbkl4OX40ZnJjZW05Y35hUGtYU3JXMXp-RDVrUHh1fllDN3o3MEx2ejE5bHRYQVFBQUFBJCQAAAAAAAAAAAEAAADprkeEt7LKwrK7xNy088WjuckAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPVpNFf1aTRXd; BIDUPSID=690F68B321216CAD38673E191C509FE9; H_PS_PSSID=18880_1431_18280_17946_19570_19805_19558_19808_19843_19901_19861_17001_15065_12252; PSTM=1463052788; HISTORY=69cd43e090d340bc11cc8484e636a104351e49; HOSUPPORT=1; PTOKEN=36e496c2bff0ad7ba87bacee8ac064ce; SAVEUSERID=bd218e320d66a6b5c3de64edb6df5a; STOKEN=a710e67bb88db3770fa4edb28d476ebcb5f2bef8c2b26bb10eb6505402fd1fe8; UBI=fi_PncwhpxZ%7ETaJc4qxjbaqXwf-0ORKP6pPgPg60BCKpMv5eun-GtaO5OWcoz0eOiEp8JiaDXMJObkfzZa3yCDR5UNObDmep3KiSFYEy0Bcu%7E9t2zGtH6dsn99tSeeSi206yeOJaKcpoAnkzKu7nEeIXXjXRQ__; USERNAMETYPE=3; TIEBAUID=4a3c80e70f5a3d7cfe3587c5; TIEBA_USERTYPE=87feed1da029e55d18a9bc37; BDSVRTM=0; BD_HOME=0; ";


    PushNewTaskInQueue(task);

#endif

//雪球
#if 0

    struct TaskXueQiuPacket *task = new struct TaskXueQiuPacket;
    task->task_id_ = 1;
    task->task_type_ = TASK_XUEQIU;
    task->pre_url_ = "https://xueqiu.com/S/YY/68644634";
    task->content_ = "最佳大数据股票投资平台 网罗一切数据 通过强大的算法和高效稳定的框架，以最快的速度完成技术指标分析 baidu.nu/w1x";
    task->cookie_ = "s=fcp16w3hx1; _sid=mP0BEblSVn3I1jV4b0GaGbsqPjMeZv; bid=7a906e5b374f3ad9d5d08fc40f09276c_io9d2xc9; webp=0; Hm_lvt_1db88642e346389874251b5a1eded6e3=1463363547; Hm_lpvt_1db88642e346389874251b5a1eded6e3=1463372109; xq_a_token=cb123cfeb3b11088022b60b138b90229f0fae5d3; xqat=cb123cfeb3b11088022b60b138b90229f0fae5d3; xq_r_token=65a6f4cce37d81779606a63714ff2abd6456ca1c; xq_is_login=1; u=2226355683; xq_token_expire=Fri%20Jun%2010%202016%2012%3A15%3A29%20GMT%2B0800%20(CST)";

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

}

} /* namespace robot_logic */
