//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月22日 Author: kerry

#ifndef KID_CRAWLER_TASK_SCHDULER_ENGINE_H_
#define KID_CRAWLER_TASK_SCHDULER_ENGINE_H_

#include <map>
#include <list>
#include "logic/auto_crawler_infos.h"
#include "crawler_schduler/crawler_schduler_engine.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "config/config.h"
#include "share/manager_info.h"
#include "crawler_task_db.h"

#define GET_COOKIES_PER_TIME	3000

class CookieManager;
class ForgeryIPManager;
class ForgeryUAManager;
class ContentManager;

namespace robot_task_logic {

struct Config;

typedef std::map<int64, base_logic::RobotTask *> TASKINFO_MAP;
typedef std::list<base_logic::RobotTask *>  TASKINFO_LIST;

class TaskSchdulerCache {
 public:
    TASKINFO_MAP          task_idle_map_;
    TASKINFO_MAP          task_exec_map_;
    TASKINFO_LIST         task_temp_list_;
};

class TaskSchdulerManager {
 public:
    TaskSchdulerManager();
    virtual ~TaskSchdulerManager();

    void Init(router_schduler::SchdulerEngine* crawler_engine, Config *config);

    void InitDB(robot_task_logic::CrawlerTaskDB*     task_db);

    void InitManagerInfo(plugin_share::ManagerInfo *info);

    void FetchBatchTask(std::list<base_logic::RobotTask *> *list,
            bool is_first = false);

    void FetchBatchTemp(std::list<base_logic::TiebaTask>* list);

 public:
    bool DistributionTask();

    void RecyclingTask();

    bool AlterTaskState(const int64 task_id, const int8 state);

    bool AlterCrawlNum(const int64 task_id, const int64 num);

    void CheckIsEffective();

    uint32 GetExecTasks();

    bool RemoveInvalidCookie(int64 cookie_id);

 private:
    void Init();

    void SetUpdateTime(const int64 plat_id, const int64 update_time);

    void PrintInfo();


 private:
    struct threadrw_t*                     lock_;
    plugin_share::ManagerInfo			   *manager_info_;
    TaskSchdulerCache*                     task_cache_;
    router_schduler::SchdulerEngine*       crawler_schduler_engine_;
    int32                                  crawler_count_;
    robot_task_logic::CrawlerTaskDB*       task_db_;
	CookieManager						   *cookie_cache_;
	ForgeryIPManager					   *ip_cache_;
	ContentManager						   *content_cache_;
	ForgeryUAManager					   *ua_cache_;
	Config								   *config_;
};

class TaskSchdulerEngine {
 private:
    static TaskSchdulerManager    *schduler_mgr_;
    static TaskSchdulerEngine     *schduler_engine_;

    TaskSchdulerEngine() {}
    virtual ~TaskSchdulerEngine() {}
 public:
    static TaskSchdulerManager* GetTaskSchdulerManager() {
        if (schduler_mgr_ == NULL)
            schduler_mgr_ = new TaskSchdulerManager();
        return schduler_mgr_;
    }

    static TaskSchdulerEngine* GetTaskSchdulerEngine() {
        if (schduler_engine_ == NULL)
            schduler_engine_ = new TaskSchdulerEngine();
        return schduler_engine_;
    }
};
}  // namespace crawler_task_logic

#endif /* TASK_SCHDULER_ENGINE_CC_ */
