//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015.9.22 Author: kerry

#ifndef KID_CRAWLER_TASK_DB_H_
#define KID_CRAWLER_TASK_DB_H_

#include <string>
#include <list>
#include <map>

#include "../../pub/logic/auto_crawler_infos0505.h"
#include "storage/storage_controller_engine.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace weibo_task_logic {

typedef std::map<int32, base_logic::TaskPlatDescription> TASKPLAT_MAP;

struct RobotCookie {
	int64		id;
	std::string	cookie;
};

struct RobotIP {
	std::string		ip;
};


class CrawlerTaskDB {
public:
   CrawlerTaskDB();
   virtual ~CrawlerTaskDB();

public:
   bool RecordRobotTaskState(base_logic::RobotTask &task);

   bool UpdateRobotTaskState(int64 task_id, int state);

   int GetCrawlerTypeByOpCode(uint32 op_code);

	bool FectchBatchForgeryIP(std::list<base_logic::ForgeryIP>* list);

   bool GetCookies(int count, uint64 last_time, std::list<base_logic::LoginCookie>* cookies_list);

   bool FetchBatchTaskContent(int16 task_type, std::list<base_logic::RobotTaskContent> *list, bool is_new = true);
public:
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
