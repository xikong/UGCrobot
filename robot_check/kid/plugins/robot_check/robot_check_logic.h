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

#include "robot_check_db.h"
#include "robot_check_header.h"

#define TASK_LOG_CHECK_INTERVAL                 10001

namespace robot_check_logic {

class RobotCheckLogic{
 private:
    RobotCheckLogic();
    bool Init();

 public:
    static RobotCheckLogic *GetInstance();
    static void FreeInstance();
    bool OnIniTimer(struct server *srv );
    bool OnTimeout(struct server *srv, char* id, int opcode, int time );
    void SaveRobotCheckPlugin(struct plugin *pl );
    struct plugin *GetRobotCheckPlugin();

    bool LoadTaskFromDb();
    void UpdateRobotTaskStatus(RobotTask &task );

 private:
    string check_date_;
    int comlete_num_;
    int load_num_;
    static RobotCheckLogic *instance_;
    scoped_ptr<RobotCheckDB> robot_check_db_;
    struct plugin *robot_check_pl_;
};

} /* namespace robot_check_logic */

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_LOGIC_H_ */
