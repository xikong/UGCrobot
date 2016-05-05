/*
 * robot_check_engine.cc
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#include "robot_check_engine.h"
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include "logic/logic_pub_comm.h"
#include "logic/logic_comm.h"
#include "net/comm_head.h"
#include "core/common.h"
#include "basic/basictypes.h"
#include "basic/template.h"
#include "basic/scoped_ptr.h"

#include "robot_check/robot_check_logic.h"

namespace robot_check_logic {

RobotCheckEngine *RobotCheckEngine::instance_ = NULL;

RobotCheckEngine::RobotCheckEngine()
 : fp_task_check_log_(NULL) {
    if(!Init()){
        assert(0);
    }
}

RobotCheckEngine::~RobotCheckEngine() {
    DeinitThreadrw(lock_);
    fclose(fp_task_check_log_);
}

bool RobotCheckEngine::Init(){

    InitThreadrw(&lock_);

    return true;
}

RobotCheckEngine *RobotCheckEngine::GetInstance(){
    if(NULL == instance_){
        instance_ = new RobotCheckEngine();
    }
    return instance_;
}

void RobotCheckEngine::FreeInstance(){
    delete instance_;
    instance_ = NULL;
}

bool RobotCheckEngine::PushNewCheckTaskInQueue(FILE *fp){

    base_logic::WLockGd lk(lock_);
    ready_check_task_queue_.push(fp);
    PostThreadTaskReady();

    return true;
}

bool RobotCheckEngine::PopCheckTaskFromQueue(FILE **fp){

    base_logic::WLockGd lk(lock_);

    *fp = ready_check_task_queue_.front();
    ready_check_task_queue_.pop();

    return true;
}

bool RobotCheckEngine::PostThreadTaskReady(){

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
    struct plugin *robot_check_pl = RobotCheckLogic::GetInstance()->GetRobotCheckPlugin();
    ret = srv->user_addtask(srv, TASK_CHECK_READY, robot_check_pl);
    if(0 != ret){
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    return true;
}

bool RobotCheckEngine::OnTaskThreadFunc(struct server *srv, int fd, void* data){

    switch(fd){
    case TASK_CHECK_READY:{
        StartTaskWork();
        break;
    }
    default:
        break;
    }

    return true;
}

void RobotCheckEngine::StartTaskWork(){
    //获取一个任务文件
    FILE *fp = NULL;
    PopCheckTaskFromQueue(&fp);
    if(NULL == fp){
        return;
    }

    //将日志文件加强制读锁
    base_logic::RFileLockGd lk(fp);

    LOG_MSG("****** Start Check TaskLogFile ****** \n");

    //处理任务文件
    DealTaskLogCheck(fp);
}

bool RobotCheckEngine::OpenTaskLogFile(const std::string &file_path,
        const int check_interval){

    //构造日志文件
    TaskLogCheckDesc log_check_info;

    log_check_info.file_path_ = file_path;
    log_check_info.next_check_time_ = 0;
    log_check_info.check_interval_ = check_interval;
    log_check_info.fp_ = fopen(file_path.c_str(), "r");
    if(NULL == log_check_info.fp_){
        LOG_MSG2("File Not Exist, file_path = %s", file_path.c_str());
        return false;
    }

    LOG_MSG2("Open RobotTaskLog File Success, filepath = %s, checkinterval = %d",
            file_path.c_str(), check_interval);

    base_logic::WLockGd lk(lock_);
    fp_task_log_map_[file_path] = log_check_info;

    return true;
}

bool RobotCheckEngine::CheckRobotTaskValid(int current_time){

    bool r = false;

    //关闭文件
    if(NULL != fp_task_check_log_){
        fclose(fp_task_check_log_);
    }

    //重新打开, 清空内容
    fp_task_check_log_ = fopen("./fail_task.log", "w");
    if(NULL == fp_task_check_log_){
        LOG_MSG("fopen fail_task_log file faild");
        return false;
    }

    LOG_MSG2("Check TaskLogFile Count = %d", fp_task_log_map_.size());
    TASK_LOG_CHECK_DESC_MAP::iterator iter = fp_task_log_map_.begin();
    for(; iter != fp_task_log_map_.end(); ++iter){

        std::string filepath = iter->second.file_path_;

        //日志文件每隔固定时间检查
        if(current_time < iter->second.next_check_time_){
            LOG_MSG2("filepath = %s, NextCheckTime = %d, current_time = %d",
                    filepath.c_str(), iter->second.next_check_time_, current_time);
            continue;
        }

        //如果文件不存在跳过
        FILE *fp = iter->second.fp_;
        if(NULL == fp){
            LOG_MSG2("TaskLogFile Not Exists, filepath = %s \n", filepath.c_str());
            continue;
        }

        //该日志文件处理的最新时间
        iter->second.next_check_time_ = current_time + iter->second.check_interval_;

        //判断文件大小
        struct stat statbuf;
        stat(iter->second.file_path_.c_str(), &statbuf);
        if(statbuf.st_size <= 0){
            LOG_MSG2("TaskLogFile is Empty, taskpath = %s, filelen = %d",
                    filepath.c_str(), statbuf.st_size);
            continue;
        }

        //添加到任务队列中
        PushNewCheckTaskInQueue(fp);
    }

    return true;
}

bool RobotCheckEngine::DealTaskLogCheck(FILE * const fp){

    //从新处理该文件
    fseek(fp, 0, SEEK_SET);

    std::stringstream os;
    char *buf = new char[8192];
    while(true){
        memset(buf, '\0', sizeof(buf));
        if (feof(fp) != 0) {
            break;
        }

        if (fgets(buf, 8191, fp) == NULL) {
            continue;
        }

        if(buf[0] != '\n'){
            os << buf;
            continue;
        }

        std::string str_task_info = os.str();
        LOG_MSG2("Read One Task Record, task_info = \n %s", str_task_info.c_str());
        os.str("");

        //执行任务检查
        ExcuteTaskCheck(str_task_info);
    }

    delete[] buf;
    return true;
}

void RobotCheckEngine::ExcuteTaskCheck(const std::string task_info){

    bool r = false;
    int task_id, task_type;
    std::string str_referer, str_content;
    r = ParseTaskInfo(task_info, task_id, task_type, str_referer, str_content);
    if(!r){
        LOG_MSG2("Parse TaskLogInfo Failed, task_info = \n\n %s", task_info.c_str());
        return;
    }

    base_logic::TaskEngine *engine = GetTaskEngineByType(task_type);
    if(NULL == engine){
        return;
    }

    std::string str_response;
    r = engine->StartTaskWork(str_referer, str_content, str_response);
    if(r){
        LOG_MSG2("task_id = %d, task_type = %d, task_content = %s In %s",
                task_id, task_type, str_content.c_str(), str_referer.c_str());
        return;
    }

    //组合日志文件
    std::stringstream os;
    os << "task_id = " << task_id << ", ";
    os << "task_type = " << task_type << ", ";
    os << "task_content = " << str_content << ", ";
    os << "Not Exists In " << str_referer;

    //组合失败发帖日志
    WriteLogFile(os.str());
}

bool RobotCheckEngine::ParseTaskInfo(const std::string &task_info,
            int &task_id, int &task_type, std::string &str_referer,
            std::string &str_content){

    bool r = false;
    std::stringstream os;

    //任务id
    std::string str_task_id;
    r = logic::SomeUtils::FindStrFromString(str_task_id, task_info, "task_id = ", ',');
    if(!r){
        LOG_MSG2("TaskCheck FindTaskId Failed, task_info = %s", task_info.c_str());
        return false;
    }

    task_id = atoi(str_task_id.c_str());
    if(0 == task_id){
        LOG_MSG2("Parse task_id failed, task_id = %s", str_task_id.c_str());
        return false;
    }

    //任务类型
    std::string str_task_type;
    r = logic::SomeUtils::FindStrFromString(str_task_type, task_info, "task_type = ", ',');
    if(!r){
        LOG_MSG2("TaskCheck FindTaskType Failed, task_info = %s", task_info.c_str());
        return false;
    }

    task_type = atoi(str_task_type.c_str());
    if(0 == task_type){
        LOG_MSG2("Parse TaskType failed, task_type = %s", str_task_type.c_str());
        return false;
    }

    //任务url
    r = logic::SomeUtils::FindStrFromString(str_referer, task_info, "task_referer = ", ',');
    if(!r){
        LOG_MSG2("Parsse TaskReferer failed, task_referer = %s", str_referer.c_str());
        return false;
    }

    //发帖内容
    r = logic::SomeUtils::FindStrFromString(str_content, task_info, "task_content = ", ',');
    if(!r){
        LOG_MSG2("Parse TaskContent failed, task_content = %s", str_content.c_str());
        return false;
    }

    return true;
}

void RobotCheckEngine::WriteLogFile(const std::string &fail_task_info){

    LOG_MSG2("%s", fail_task_info.c_str());

    //将日志文件加锁
    base_logic::WFileLockGd lk(fp_task_check_log_);

    //将任务信息写到日志中
    fseek(fp_task_check_log_, 0, SEEK_END);
    if ( fwrite (fail_task_info.c_str(), fail_task_info.size(), 1, fp_task_check_log_) < 0) {
        LOG_MSG("fwrite response error");
    }

    fflush(fp_task_check_log_);
}


} /* namespace robot_check_logic */
