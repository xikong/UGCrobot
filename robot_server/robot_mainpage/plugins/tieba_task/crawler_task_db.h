//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015.9.22 Author: kerry

#ifndef KID_CRAWLER_TASK_DB_H_
#define KID_CRAWLER_TASK_DB_H_

#include <string>
#include <list>
#include <map>
#include "storage/storage_controller_engine.h"
#include "logic/auto_crawler_infos.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace tieba_task_logic {

class CrawlerTaskDB {
 public:
    CrawlerTaskDB();
    virtual ~CrawlerTaskDB();

 public:
    bool FetchKwBlacklist(std::list<base_logic::BlackKw> &list);

    bool FecthBatchTask(std::list<base_logic::TiebaTask>* list,
            const bool is_new = false);

    bool RecordRobotTaskState(base_logic::RobotTask &task);

    bool UpdateRobotTaskState(int64 task_id, int state);

    int GetCrawlerTypeByOpCode(uint32 op_code);

	bool FectchBatchForgeryIP(std::list<base_logic::ForgeryIP>* list);

    bool GetCookies(int count, uint64 last_time, std::list<base_logic::LoginCookie>* cookies_list);

    bool ExistMainpageTask(const std::string &rowkey, int &affected_rows);

    bool FetchLastTask(base_logic::TiebaTask &task);

    bool FectchBatchForgeryUA(std::list<base_logic::ForgeryUA>* list);

 public:
    static void CallBackFetchKwBlacklist(void* param,
                base_logic::Value* value);

    static void CallBackFectchBatchForgeryUA(void* param,
                base_logic::Value* value);

    static void CallBackFectchBatchTask(void* param,
            base_logic::Value* value);

    static void CallBackGetCookies(void* param,
            base_logic::Value* value);

    static void CallBackGetCrawlerType(void *param,
    		base_logic::Value *value);

	static void CallBackFectchBatchForgeryIP(void* param,
	            base_logic::Value* value);

	static void CallBackGetTaskId(void* param,
	            base_logic::Value* value);

	static void CallBackAffectedRow(void* param,
	            base_logic::Value* value);

	static void CallBackFetchLastTask(void* param,
	            base_logic::Value* value);
 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};
}  // namespace crawler_task_logic


#endif /* TASK_DB_H_ */
