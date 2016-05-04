/*
 * robot_check_logic.h
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_LOGIC_H_
#define KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_LOGIC_H_

#include "core/common.h"
#include "basic/basictypes.h"

#define TASK_LOG_CHECK_INTERVAL                 10001

namespace robot_check_logic {

class RobotCheckLogic {
 public:
    virtual ~RobotCheckLogic();

    static RobotCheckLogic *GetInstance();

    static void FreeInstance();

    bool OnIniTimer(struct server *srv);

    bool OnTimeout(struct server *srv, char* id, int opcode, int time);

    void TaskLogCheckTimer(int current_time);

    void SaveRobotCheckPlugin(struct plugin *pl);

    struct plugin *GetRobotCheckPlugin();

 private:
    RobotCheckLogic();

    bool Init();

 private:
    static RobotCheckLogic *instance_;
    struct plugin           *robot_check_pl_;
    int64                   next_check_time_;
    bool                    is_parse_finsh_;
};

} /* namespace robot_check_logic */

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_LOGIC_H_ */
