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

    fp_task_log_ = fopen("./task.log", "w");
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

    LOG_MSG2("Add New Task In Queue, task_id = %u, task_type = %d, curr_task_num = %d",
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
//  r = engine->StartTaskWork(task, str_response);

    //任务结果写人日志文件
    WriteLogFile(task, r);

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

bool RobotEngine::WriteLogFile(struct TaskHead *task, bool is_success){

    base_logic::WLockGd lk(lock_);

    //组合日志
    std::stringstream os;
    os << "task_id = " << task->task_id_ << "   ";
    os << "task_type = " << task->task_type_ << "\n";
    os << "is_success = \n" << is_success << "\n\n";

    //将任务信息写到日志中
    string str_log = os.str();
    fseek(fp_task_log_, 0, SEEK_END);
    if ( fwrite (str_log.c_str(), str_log.size(), 1, fp_task_log_) < 0) {
        LOG_MSG("fwrite response error");
    }

    fflush(fp_task_log_);
}


} /* namespace robot_logic */
