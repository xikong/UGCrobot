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

    LOG_MSG2("Pop New Task From Queue, task_id = %u, task_type = %d, curr_task_num = %d",
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

    //任务结果写人日志文件
    WriteLogFile(task, str_response);

    //向服务器反馈任务的状态
    FeedBackTaskStatus(task, r);

    LOG_MSG("TaskWork Job finsh");

    return;
}

bool RobotEngine::FeedBackTaskStatus(struct TaskHead *task, bool is_success){

    bool r = false;

    struct FeedBackTaskStatus feedback_status_msg;
    MAKE_HEAD(feedback_status_msg, C2S_FEEDBACK_TASK_STATUS, 0, 0, 0, 0);
    feedback_status_msg.is_success = TASK_SUCCESS;
    if(!is_success){
        feedback_status_msg.is_success = TASK_FAIL;
    }

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

    LOG_MSG2("Send FeedBackTaskStatus Msg Success, feedback_server_id = %d,"
                    " task_id = %d, task_type = %d, is_success = %d \n",
                    task->feed_server_id_,
                    task->task_id_,
                    task->task_type_,
                    feedback_status_msg.is_success);

    return true;
}

bool RobotEngine::WriteLogFile(struct TaskHead *task, const string &response){

    //将日志文件加锁
    base_logic::WFileLockGd lk(fp_task_log_);

    //组合日志
    std::stringstream os;
    os << logic::SomeUtils::GetLocalTime(time(NULL)) << ", ";
    os << "task_id = " << task->task_id_ << ", ";
    os << "task_type = " << task->task_type_ << ", \n";
    os << "task_referer = " << task->pre_url_ << ", \n";
    os << "task_content = " << task->content_ << ", \n";
    os << "response = " << response << "\n\n";

    //将任务信息写到日志中
    string str_log = os.str();
    fseek(fp_task_log_, 0, SEEK_END);
    if ( fwrite (str_log.c_str(), str_log.size(), 1, fp_task_log_) < 0) {
        LOG_MSG("fwrite response error");
    }

    fflush(fp_task_log_);
}

void RobotEngine::Test(){

    struct TaskTaoGuBaPacket *task = new struct TaskTaoGuBaPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_TAOGUBA;

    task->content_ = "不错";
    task->topicID_ = "1473659";
    task->subject_ = "商人的几个类型与商业的几个模式";
    task->cookie_ = "JSESSIONID=2F2D6FC9062DAEA5D0B46ACA6D539E04-n1; CNZZDATA1574657=cnzz_eid%3D1151724852-1462413656-http%253A%252F%252Fwww.baidu.com%252F%26ntime%3D1462413656; tgbuser=1693199; tgbpwd=401069A9724aud51rvfq2rzrxh; bdshare_firstime=1462417196359";

//贴吧
#if 0
    struct TaskTieBaPacket *task = new struct TaskTieBaPacket;

    task->task_id_ = 1;
    task->task_type_ = TASK_TIEBA;

    task->pre_url_ = "http://tieba.baidu.com/p/4521147288";
    task->kw_ = "htcx9";
    task->fid_ = "21618411";
    task->content_ = "不错";

    task->cookie_ = "tbs=71c1877d94e08c5e1462258768;BAIDUID=092C32B642E358C256261F8CCC33B3E7:FG=1; BDUSS=B-cXlMb0c0bWRieTFwUHE4TVVMWE5rUDl-QjRoVzdWenRoaFh6aW5FRlEyVTlYQVFBQUFBJCQAAAAAAAAAAAEAAAAWyMKBbHR6bnA3MTU3NwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFBMKFdQTChXeF; BIDUPSID=092C32B642E358C256261F8CCC33B3E7; H_PS_PSSID=18880_18286_1424_19782_17944_19805_19900_19559_19807_19843_19901_18134_17001_15540_11503_10634; PSTM=1462258767; HISTORY=213f20f52d8bf07c04ab5732fab36fee721748; HOSUPPORT=1; PTOKEN=e6602542d18203454b19d23798c17f7f; SAVEUSERID=1109a219a93d862ac01ce9c8dac65b; STOKEN=0005e74cc3d51fe39c427f309616e749cf95059e4d8c9614d374cf6f3183e8d2; UBI=fi_PncwhpxZ%7ETaJczCAqA7NWC58pA1B%7EtUdrF1wxzXKaI9hPoHEoJwYtUM43Xfhn1NyESfr5WbJzRah7-bBa9upeuaVarXbVbmI7Cj7xXdc7QvbQt066an7cxW4uOromnwF76MGolHVH-AdbpswwDMG1IvFww__; USERNAMETYPE=3; TIEBAUID=a320d01d2ee5262b877d6fbf; TIEBA_USERTYPE=f6f1ec33f1ac043f8929cf55; BDSVRTM=0; BD_HOME=0;";
#endif

    PushNewTaskInQueue(task);
}

} /* namespace robot_logic */
