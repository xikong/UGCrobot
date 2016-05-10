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

namespace crawler_task_logic {

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
    bool FecthBatchTask(std::list<base_logic::TaskInfo>* list,
            const bool is_new = false);

    bool RecordTaskState(base_logic::TaskInfo& task, const int32 type);

    bool RecordRobotTaskResult(int64 task_id, int is_success);

    bool GetTaskPlatTaskDescription(
            std::list<base_logic::TaskPlatDescription>* list);

    void BatchFectchTaskPlatInfo(
            std::list<base_logic::TaskPlatDescription>* list);

    void BatchUpdateTaskInfo(std::list<base_logic::TaskInfo>* list);

    int GetCrawlerTypeByOpCode(uint32 op_code);

	int RecordWeiboTaskState(base_logic::WeiboTaskInfo &task);

	bool FetchRobotCookies(std::list<struct RobotCookie> *list, int64 attr_id, int count);

	bool FectchBatchForgeryIP(std::list<base_logic::ForgeryIP>* list);

	// 
    bool GetCookie(std::list<base_logic::LoginCookie>* cookie_list, const int64 id,
            const int64 from, const int64 count, int64& plat_update_time);

    bool GetCookies(int count, uint64 last_time, std::list<base_logic::LoginCookie>* cookies_list);

 public:
    static void CallBackGetCookie(void* param,
            base_logic::Value* value);

    static void CallBackGetCookies(void* param,
            base_logic::Value* value);

    static void CallBackFectchBatchTask(void* param,
            base_logic::Value* value);

    static void CallBackFectchBatchTempTask(void* param,
            base_logic::Value* value);

    static void CallBackGetTaskPlatDescription(void* param,
                base_logic::Value* value);

    static void CallBackGetCrawlerType(void *param,
    		base_logic::Value *value);

    static void CallBackRecordWeiboTaskState(void *param,
    		base_logic::Value *value);

    static void CallBackFetchRobotCookies(void *param,
    		base_logic::Value *value);

    static void  CallBackFetchRobotIP(void *param,
    		base_logic::Value *value);

	static void CallBackFectchBatchForgeryIP(void* param,
	            base_logic::Value* value);
 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
    TASKPLAT_MAP   task_platform_;
    bool task_platform_inited_;
};
}  // namespace crawler_task_logic


#endif /* TASK_DB_H_ */
