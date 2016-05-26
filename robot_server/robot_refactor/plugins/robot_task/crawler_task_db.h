//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015.9.22 Author: kerry

#ifndef KID_CRAWLER_TASK_DB_H_
#define KID_CRAWLER_TASK_DB_H_

#include <string>
#include <list>
#include <map>

#include "logic/auto_crawler_infos.h"
#include "storage/storage_controller_engine.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace robot_task_logic {

class CrawlerTaskDB {
 public:
    CrawlerTaskDB();
    virtual ~CrawlerTaskDB();

 public:
    bool UpdateCookie(int64 cookie_id, int is_valid);

    bool UpdateCookieAccessTime(int64 cookie_id, time_t last_access_time);

    bool BindIPToCookie(int64 cookie_id, int64 ip_id, const std::string &ip);

    bool BindUAToCookie(int64 cookie_id, int64 ua_id);

    bool FectchBatchForgeryUA(std::list<base_logic::ForgeryUA>* list);

    bool ExistRobotTask(base_logic::RobotTask *task, int &affected_rows);

    bool RecordRobotTaskState(base_logic::RobotTask *task);

    bool UpdateRobotTaskDetail(base_logic::RobotTask *task);

    bool RecordRobotTasks(std::list<base_logic::RobotTask *> &list);

    bool UpdateRobotTaskState(int64 task_id, int state, std::string error_code = "none");

    int GetCrawlerTypeByOpCode(uint32 op_code);

	bool FectchBatchForgeryIP(std::list<base_logic::ForgeryIP>* list);

    bool GetCookies(int count, uint64 attr_id, uint64 usable_time, std::list<base_logic::LoginCookie>* cookies_list);

    bool FetchBatchTaskContent(int16 task_type, std::list<base_logic::RobotTaskContent> *list, bool is_new = false);
 public:
    static void CallBackFectchBatchForgeryUA(void* param,
                base_logic::Value* value);

    static void CallBackAffectedRow(void* param, base_logic::Value* value);

    static void CallBackGetCookies(void* param,
            base_logic::Value* value);

    static void CallBackGetCrawlerType(void *param,
    		base_logic::Value *value);

	static void CallBackFectchBatchForgeryIP(void* param,
	            base_logic::Value* value);

	static void CallBackGetTaskId(void* param,
	            base_logic::Value* value);

	static void CallBackFetchBatchContent(void* param,
            base_logic::Value* value);
 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};
}  // namespace crawler_task_logic


#endif /* TASK_DB_H_ */
