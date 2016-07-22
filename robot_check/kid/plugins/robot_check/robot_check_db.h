/*
 * robot_check_db.h
 *
 *  Created on: 2016年7月18日
 *      Author: harvey
 */

#ifndef KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_DB_H_
#define KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_DB_H_

#include "robot_check_header.h"

namespace robot_check_logic {

class RobotCheckDB{
 public:
    RobotCheckDB();
    bool FetchTodayTask(std::list<RobotTask *> &list,
                        const string &check_date );
    bool RecordRobotTaskStatus(RobotTask &task );
 private:
    static void CallbackFetchTodayTask(void* param, base_logic::Value* value );

 private:
    string GetTodoyStr() {
        char tmp[64];
        time_t now = time(NULL);
        strftime(tmp, sizeof(tmp), "%Y-%m-%d", localtime(&now));
        return string(tmp);
    }

 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};

} /* namespace robot_check_logic */

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_DB_H_ */
