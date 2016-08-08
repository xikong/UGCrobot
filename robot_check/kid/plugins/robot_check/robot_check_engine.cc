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

RobotCheckEngine::RobotCheckEngine() {
    InitThreadrw(&lock_);
}

RobotCheckEngine::~RobotCheckEngine() {
    DeinitThreadrw(lock_);
}

RobotCheckEngine *RobotCheckEngine::GetInstance() {
    if (NULL == instance_) {
        instance_ = new RobotCheckEngine();
    }
    return instance_;
}

void RobotCheckEngine::FreeInstance() {
    delete instance_;
    instance_ = NULL;
}

bool RobotCheckEngine::PushNewCheckTaskInQueue(RobotTask *task ) {
    base_logic::WLockGd lk(lock_);
    ready_check_task_queue_.push(task);
    PostThreadTaskReady(TASK_CHECK_READY);
    return true;
}

bool RobotCheckEngine::PopCheckTaskFromQueue(RobotTask **task ) {
    base_logic::WLockGd lk(lock_);
    *task = ready_check_task_queue_.front();
    ready_check_task_queue_.pop();
    return true;
}

bool RobotCheckEngine::PostThreadTaskReady(int witch ) {
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
    struct plugin *robot_check_pl = RobotCheckLogic::GetInstance()
            ->GetRobotCheckPlugin();
    ret = srv->user_addtask(srv, witch, robot_check_pl);
    if (0 != ret) {
        LOG_MSG("PostThreadTaskReady srv->user_addtask Failed");
        return false;
    }

    return true;
}

bool RobotCheckEngine::OnTaskThreadFunc(struct server *srv, int fd,
                                        void* data ) {
    switch (fd) {
        case TASK_CHECK_READY: {
            StartCheckTaskWork();
            break;
        }
        case TASK_CHECK_COMPLETE: {
            RecordTaskStatus();
            break;
        }
        default:
            break;
    }

    return true;
}

void RobotCheckEngine::StartCheckTaskWork() {
    RobotTask *task = NULL;
    PopCheckTaskFromQueue(&task);
    if ( NULL == task) {
        return;
    }
    base_logic::TaskEngine *engine = GetTaskEngineByType(task->task_type_);
    if (NULL == engine) {
        delete task;
        return;
    }
    bool r = engine->StartTaskWork(*task);
    if (r) {
        task->is_sucess_ = TASK_SUCCESS;
    }
    AddCompleteTaskInMap(task);
    if (GetWaitCheckQueueSize() <= 0) {
        PostThreadTaskReady(TASK_CHECK_COMPLETE);
    }
}

bool RobotCheckEngine::AddCompleteTaskInMap(RobotTask *task ) {
    base_logic::WLockGd lk(lock_);
    complete_robot_task_list_.push_back(task);
    return true;
}

int RobotCheckEngine::GetWaitCheckQueueSize() {
    base_logic::WLockGd lock(lock_);
    return ready_check_task_queue_.size();
}

void RobotCheckEngine::RecordTaskStatus() {
    base_logic::WLockGd lk(lock_);
    while (complete_robot_task_list_.size() > 0) {
        RobotTask *task = complete_robot_task_list_.front();
        complete_robot_task_list_.pop_front();
        RobotCheckLogic::GetInstance()->UpdateRobotTaskStatus(*task);
        delete task;
    }
}

} /* namespace robot_check_logic */
