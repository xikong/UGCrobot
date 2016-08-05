//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月22日 Author: kerry

#ifndef KID_CRAWLER_TASK_SCHDULER_ENGINE_H_
#define KID_CRAWLER_TASK_SCHDULER_ENGINE_H_

#include <map>
#include <list>
#include <sstream>
#include "crawler_schduler/crawler_schduler_engine.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "config/config.h"

#include "logic/auto_crawler_infos.h"
#include "share/session_engine.h"
#include "crawler_task_db.h"
#include "kw_blacklist.h"

#define GET_COOKIES_PER_TIME	3000

class ForgeryIPManager;

namespace tieba_task_logic {

typedef std::map<int64, base_logic::TiebaTask> TASKINFO_MAP;
typedef std::list<base_logic::TiebaTask> TASKINFO_LIST;

struct TaskList {
  int64 attr_id_;
  TASKINFO_LIST main_list_;
  TASKINFO_LIST middle_list_;
  TASKINFO_LIST final_list_;
  void ClearMainTaskList() { main_list_.clear(); }
  void push_back(base_logic::TiebaTask &task) {
    std::string type;
    if (base_logic::RobotTask::MAIN == task.sub_type()) {
      type = "main";
      main_list_.push_back(task);
    } else if (base_logic::RobotTask::MIDDLE == task.sub_type()) {
      type = "middle";
      middle_list_.push_back(task);
    } else {
      type = "final";
      final_list_.push_back(task);
    }
    LOG_DEBUG2("%s --> %s", task.url().c_str(), type.c_str());
  }
  void push_main_task(base_logic::TiebaTask &task) {
    main_list_.push_back(task);
  }
  size_t size() {
    return main_list_.size() + middle_list_.size() + final_list_.size();
  }
  bool GetTask(base_logic::TiebaTask &task) {
    time_t now = time(NULL);
    do {
      if (final_list_.size() > 0) {
        task = final_list_.front();
        final_list_.pop_front();
      } else if (middle_list_.size() > 0) {
        task = middle_list_.front();
        middle_list_.pop_front();
      } else if (main_list_.size() > 0) {
        task = main_list_.front();
        main_list_.pop_front();
        main_list_.push_back(task);
      } else {
        return false;
      }

      if (task.is_timeout()) {
        LOG_DEBUG2("task timeout, url = %s, current_time = %d, create_time = %d",
           task.url().c_str() , now, task.create_time());
      }
    } while (task.is_timeout());
    return true;
  }
};

typedef std::map<int64, TaskList> PendingTaskMap;
class TaskSchdulerCache {
 public:
  TASKINFO_LIST task_idle_list_;
  PendingTaskMap pending_task_map_;
  TASKINFO_MAP task_exec_map_;
  TASKINFO_LIST task_temp_list_;

  void ClearMainTask() {
    PendingTaskMap::iterator it = pending_task_map_.begin();
    for (; it != pending_task_map_.end(); ++it) {
      it->second.ClearMainTaskList();
    }
  }

  void PrintTaskDetail() {
    //******************* DEBUG *******************
    PendingTaskMap::iterator it = pending_task_map_.begin();
    std::stringstream os;
    os << "\n---------------------- task detail begin ----------------------\n";
    for (; it != pending_task_map_.end(); ++it) {
      os << "type: " << it->first << ", ";
      os << "main: " << it->second.main_list_.size() << ", ";
      os << "middle: " << it->second.middle_list_.size() << ", ";
      os << "final: " << it->second.final_list_.size() << "\n";
    }
    os << "---------------------- task detail end  ----------------------";
    LOG_DEBUG2("%s", os.str().c_str());
    //******************* DEBUG *******************
  }
  bool GetTask(base_logic::TiebaTask &task) {
    base_logic::TiebaTask::TypeList &type_list = task.type_list_;
    base_logic::TiebaTask::TypeList::iterator it = type_list.begin();
    for (; it != type_list.end(); ++it) {
      int64 type = *it;
      if (pending_task_map_[type].size() > 0) {
        type_list.erase(it);
        type_list.push_back(type);
        return pending_task_map_[type].GetTask(task);
      }
    }
    return false;
  }
  size_t size() {
    size_t sum = 0;
    PendingTaskMap::iterator it = pending_task_map_.begin();
    for (; it != pending_task_map_.end(); ++it) {
      sum += it->second.size();
    }
    return sum;
  }
};

typedef std::list<base_logic::LoginCookie> CookieList;

struct CookiePlatform {
  CookieList list;
  CookieList::iterator cur_it;
  time_t update_time_;
  CookiePlatform() {
    cur_it = list.end();
    update_time_ = 0;
  }
};

typedef std::map<int64, CookiePlatform> CookieMap;

class CookieCache {
 public:
  ~CookieCache() {
//    DeinitThreadMutex(lock_);
  }
  bool GetCookie(int64 attr_id, base_logic::LoginCookie &cookie) {
//    base_logic::MLockGd lk(lock_);
    if (cookie_map_.end() == cookie_map_.find(attr_id)) {
      LOG_MSG2("can't find the cookie with the attr_id: %d", attr_id);
      return false;
    }
    struct CookiePlatform &platform = cookie_map_[attr_id];
    if (platform.list.end() == platform.cur_it)
      platform.cur_it = platform.list.begin();
    cookie = *platform.cur_it++;
    cookie.update_time();
    return true;
  }

  void SortCookies() {
//    base_logic::MLockGd lk(lock_);
    CookieMap::iterator it = cookie_map_.begin();
    for (; cookie_map_.end() != it; ++it) {
      struct CookiePlatform &platform = it->second;
      platform.list.sort(base_logic::LoginCookie::cmp);
      platform.cur_it = platform.list.begin();
    }
  }
 public:
//  struct threadmutex_t *lock_;
  CookieMap cookie_map_;
  std::map<int64, int64> update_time_map_;
  uint64 last_time;
  CookieCache() {
    last_time = 0;
//    InitThreadMutex(&lock_, PTHREAD_MUTEX_RECURSIVE);
  }
};

class IPCache {
 public:
  IPCache() {
    cur_it_ = ip_list_.end();
  }
  bool GetIP(base_logic::ForgeryIP &ip) {
    if (ip_list_.empty()) {
      LOG_MSG("there is NONE ip");
      return false;
    }
    if (ip_list_.end() == cur_it_)
      cur_it_ = ip_list_.begin();
    ip = *cur_it_++;
    ip.update_time();
    return true;
  }

  void SortIPBySendTime() {
    ip_list_.sort(base_logic::ForgeryIP::cmp);
    cur_it_ = ip_list_.begin();
  }
 private:
  friend class TaskSchdulerManager;
  std::list<base_logic::ForgeryIP> ip_list_;
  std::list<base_logic::ForgeryIP>::iterator cur_it_;
};

class ForgeryUACache {
 public:
  ForgeryUACache() {
    cur_it_ = ua_list_.end();
  }
  bool GetUA(base_logic::ForgeryUA &ua) {
    if (ua_list_.empty()) {
      LOG_MSG("there is NONE ua");
      return false;
    }
    if (ua_list_.end() == cur_it_) {
      cur_it_ = ua_list_.begin();
    }
    ua = *cur_it_++;
    ua.update_send_time();
    return true;
  }
  void SortUABySendTime() {
    ua_list_.sort(base_logic::ForgeryUA::cmp);
    cur_it_ = ua_list_.begin();
  }
 private:
  friend class TaskSchdulerManager;
  std::list<base_logic::ForgeryUA> ua_list_;
  std::list<base_logic::ForgeryUA>::const_iterator cur_it_;
};

class TaskSchdulerManager {
 public:
  TaskSchdulerManager();
  virtual ~TaskSchdulerManager();

  void Init(router_schduler::SchdulerEngine* crawler_engine);

  void InitDB(tieba_task_logic::CrawlerTaskDB* task_db);

  void InitManagerInfo(plugin_share::SessionManager *session_mgr);

  void FetchBatchTask(std::list<base_logic::TiebaTask> *list, bool is_first =
                          false);

  void FetchMainTask(std::list<base_logic::TiebaTask>* list);

 public:
  bool DistributionTask();

  bool DistributionTempTask();

  void RecyclingTask();

  bool AlterTaskState(const int64 task_id, const int state);

  bool AlterCrawlNum(const int64 task_id, const int64 num);

  void CheckIsEffective();

  uint32 GetExecTasks();

  int64& GetDatabaseUpdateTimeByPlatId(const int64 plat_id);

  void SetBatchCookies();

  void FetchIP();

 private:
  void Init();

  void SetCookie(const base_logic::LoginCookie& info);

  void CheckBatchCookie(const int64 plat_id);

  void SetBatchCookie(const int64 plat_id, const int64 from);

  bool FectchBacthCookies(const int64 plat_id, const int64 count,
                          std::list<base_logic::LoginCookie>* list);

  void FecthAndSortCookies(const int64 count,
                           std::list<base_logic::LoginCookie>& src_list,
                           std::list<base_logic::LoginCookie>* dst_list,
                           int64 plat_id);

  void SetUpdateTime(const int64 plat_id, const int64 update_time);

  void PrintInfo();

  bool FilterTask(base_logic::TiebaTask &task);
 private:
  struct threadrw_t* lock_;
  plugin_share::SessionManager *session_mgr_;
  TaskSchdulerCache* task_cache_;
  router_schduler::SchdulerEngine* crawler_schduler_engine_;
  int32 crawler_count_;
  tieba_task_logic::CrawlerTaskDB* task_db_;
  CookieCache *cookie_cache_;
  ForgeryIPManager *ip_cache_;
  ForgeryUACache *ua_cache_;
  KwBlacklist* kw_blacklist_;
};

class TaskSchdulerEngine {
 private:
  static TaskSchdulerManager *schduler_mgr_;
  static TaskSchdulerEngine *schduler_engine_;

  TaskSchdulerEngine() {
  }
  virtual ~TaskSchdulerEngine() {
  }
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
