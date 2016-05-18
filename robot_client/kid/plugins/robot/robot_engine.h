/*
 * robot_engine.h
 *
 *  Created on: 2016年4月7日
 *      Author: Harvey
 */

#ifndef UGCROBOT_MASTER_PLUGINS_ROBOT_ROBOT_ENGINE_H_
#define UGCROBOT_MASTER_PLUGINS_ROBOT_ROBOT_ENGINE_H_

#include <map>
#include <queue>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include "core/common.h"
#include "net/comm_head.h"
#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "task/task_engine.h"

#define TASK_READY  10001

namespace robot_logic {

typedef std::queue<struct TaskHead*> READY_TASK_QUEUE;

class RobotEngine {
 private:
    RobotEngine();
    bool Init();
    bool WriteLogFile(struct TaskHead *task, const string &response);

 public:
    virtual ~RobotEngine();

    static RobotEngine *GetInstance();
    static void FreeInstance();

    bool PushNewTaskInQueue(struct TaskHead *task);

    bool PopNewTaskFromQueue(struct TaskHead **task);

    int32 GetCurrTaskQueueNum();

    bool PostThreadTaskReady();

    bool OnTaskThreadFunc(struct server *srv, int fd, void* data);

    void StartTaskWork();

    bool MultiRobotTask(const void *msg, int32 len);

    bool FeedBackTaskStatus(struct TaskHead *task, bool is_success);

    void Test();

 private:
    static RobotEngine  *instance_;
    struct threadrw_t*  lock_;
    READY_TASK_QUEUE    g_ready_task_queue_;
    FILE                *fp_task_log_;
};

} /* namespace robot_logic */

extern base_logic::TaskEngine *GetTaskEngineByType(const int task_type);

#endif /* UGCROBOT_MASTER_PLUGINS_ROBOT_ROBOT_ENGINE_H_ */
