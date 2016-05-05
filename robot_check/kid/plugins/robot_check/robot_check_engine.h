/*
 * robot_check_engine.h
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_
#define KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_

#include <map>
#include <queue>
#include <fcntl.h>
#include <sys/file.h>

#include "core/common.h"
#include "net/comm_head.h"
#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "task/task_engine.h"

#define TASK_CHECK_READY    10001

namespace robot_check_logic {

typedef struct TaskLogInfo{
    FILE            *fp_;
    std::string     file_path_;
    int             check_interval_;
    int             next_check_time_;
}TaskLogCheckDesc;

typedef std::map<std::string, TaskLogCheckDesc> TASK_LOG_CHECK_DESC_MAP;
typedef std::queue<FILE *>  READY_CHECK_TASK_QUEUE;

class RobotCheckEngine {
 public:
    virtual ~RobotCheckEngine();

    static RobotCheckEngine *GetInstance();
    static void FreeInstance();

    bool OpenTaskLogFile(const std::string &file_path,
            const int check_interval);

    bool CheckRobotTaskValid(int time);

    bool DealTaskLogCheck(FILE * const fp);

    void ExcuteTaskCheck(const std::string task_info);

    bool ParseTaskInfo(const std::string &task_info,
            int &task_id, int &task_type, std::string &str_referer,
            std::string &str_content);

    bool OnTaskThreadFunc(struct server *srv, int fd, void* data);


 private:
    RobotCheckEngine();

    bool Init();

    void WriteLogFile(const std::string &fail_task_info);

    bool PushNewCheckTaskInQueue(FILE *fp);

    bool PopCheckTaskFromQueue(FILE **fp);

    bool PostThreadTaskReady();

    void StartTaskWork();

 private:

    struct threadrw_t       *lock_;
    FILE                    *fp_task_check_log_;

    TASK_LOG_CHECK_DESC_MAP fp_task_log_map_;
    READY_CHECK_TASK_QUEUE  ready_check_task_queue_;

    static RobotCheckEngine *instance_;
};

} /* namespace robot_check_logic */

extern base_logic::TaskEngine *GetTaskEngineByType(const int task_type);

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_ENGINE_H_ */
