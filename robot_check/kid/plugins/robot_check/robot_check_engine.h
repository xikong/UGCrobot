/*
 * robot_check_engine.h
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_
#define KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_

#include <map>
#include <list>
#include <queue>
#include <fcntl.h>
#include <sys/file.h>

#include "core/common.h"
#include "net/comm_head.h"
#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "task/task_engine.h"

#define TASK_CHECK_READY        10001
#define TASK_CHECK_COMPLETE     10002

using std::string;
namespace robot_check_logic {

typedef std::queue<RobotTask*> READY_CHECK_TASK_QUEUE;
typedef std::list<RobotTask *> COMPLETE_ROBOT_TASK_LIST;

class RobotCheckEngine{
 public:
    virtual ~RobotCheckEngine();
    static RobotCheckEngine *GetInstance();
    static void FreeInstance();

    bool OnTaskThreadFunc(struct server *srv, int fd, void* data );
    bool PushNewCheckTaskInQueue(RobotTask *task );

 private:
    RobotCheckEngine();
    bool PopCheckTaskFromQueue(RobotTask **task );
    bool PostThreadTaskReady(int witch );
    void StartCheckTaskWork();

 private:
    bool AddCompleteTaskInMap(RobotTask *task );
    int GetWaitCheckQueueSize();
    void RecordTaskStatus();

 private:

    struct threadrw_t *lock_;
    READY_CHECK_TASK_QUEUE ready_check_task_queue_;
    COMPLETE_ROBOT_TASK_LIST complete_robot_task_list_;
    static RobotCheckEngine *instance_;
};

} /* namespace robot_check_logic */

extern base_logic::TaskEngine *GetTaskEngineByType(const int task_type );

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_ */
