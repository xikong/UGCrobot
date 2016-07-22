//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry

#include <string>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"
#include "net/packet_processing.h"
#include "crawler_task_logic.h"
#include "forgery_ip_engine.h"
#include "forgery_ua_engine.h"
#include "cookie_engine.h"
#include "content_engine.h"

namespace robot_task_logic {

using namespace base_logic;

TaskSchdulerManager* TaskSchdulerEngine::schduler_mgr_ = NULL;
TaskSchdulerEngine* TaskSchdulerEngine::schduler_engine_ = NULL;

TaskSchdulerManager::TaskSchdulerManager()
    : crawler_count_(0),
      task_db_(NULL),
      manager_info_(NULL) {
  task_cache_ = new TaskSchdulerCache();
  cookie_cache_ = CookieEngine::GetCookieManager();
  ip_cache_ = ForgeryIPEngine::GetForgeryIPManager();
  content_cache_ = ContentEngine::GetContentManager();
  ua_cache_ = ForgeryUAEngine::GetForgeryUAManager();
  Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
  DeinitThreadrw (lock_);
}

void TaskSchdulerManager::Init() {
  InitThreadrw (&lock_);
}

void TaskSchdulerManager::InitDB(robot_task_logic::CrawlerTaskDB* task_db) {
  task_db_ = task_db;

  // ip 在定时任务中更新
  ip_cache_->Init(task_db);

  // UA 只获取一次
  ua_cache_->Init(task_db);
}

void TaskSchdulerManager::Init(router_schduler::SchdulerEngine* crawler_engine,
                               Config *config) {
  crawler_schduler_engine_ = crawler_engine;
  config_ = config;
  cookie_cache_->Init(config, task_db_);
  content_cache_->Init(config, task_db_);
}

void TaskSchdulerManager::InitManagerInfo(plugin_share::ManagerInfo *info) {
  manager_info_ = info;
}

void TaskSchdulerManager::FetchBatchTask(
    std::list<base_logic::RobotTask *> *list, bool is_first) {
  base_logic::WLockGd lk(lock_);
  time_t current_time = time(NULL);
  while ((*list).size() > 0) {
    base_logic::RobotTask *info = (*list).front();
    (*list).pop_front();
    task_cache_->task_idle_map_[info->id()] = info;
  }
}

bool TaskSchdulerManager::AlterTaskState(const int64 task_id,
                                         const int8 state) {
  base_logic::WLockGd lk(lock_);
  if (task_cache_->task_exec_map_.end()
      == task_cache_->task_exec_map_.find(task_id)) {
    LOG_MSG2("don't find the task, task_id = %lld", task_id);
    return false;
  }
  task_cache_->task_exec_map_[task_id]->set_state((TaskState) state);
  if (TASK_EXECUED == state) {
    task_cache_->task_exec_map_.erase(task_id);
    LOG_DEBUG2("after erase, task_exec_map size = %d",
               task_cache_->task_exec_map_.size());
  }
  return true;
}

void TaskSchdulerManager::RecyclingTask() {
  base_logic::WLockGd lk(lock_);
  TASKINFO_MAP::iterator it = task_cache_->task_exec_map_.begin();
  time_t current_time = time(NULL);
  for (; it != task_cache_->task_exec_map_.end();) {
    base_logic::RobotTask *task = it->second;
    if ((task->send_time() + 60) < current_time) {
      task_db_->UpdateRobotTaskState(task->id(), (int) TASK_WAIT,
                                     std::string("none"));
      delete it->second;
      task_cache_->task_exec_map_.erase(it++);
      LOG_DEBUG2("after erase, task_exec_map size = %d",
                 task_cache_->task_exec_map_.size());
    } else {
      it++;
    }
  }
}

bool TaskSchdulerManager::DistributionTask() {
  if (NULL == manager_info_) {
    LOG_MSG("wait server init complete");
    return false;
  }

  base_logic::WLockGd lk(lock_);
  int32 base_num = 1;
  int data_length = 0;
  time_t current_time = time(NULL);
  LOG_DEBUG2("distrubute task current_time=%d task_cache_->task_idle_map_.size=%d",
      (int) current_time, task_cache_->task_idle_map_.size());
  if (task_cache_->task_idle_map_.size() <= 0) {
    return true;
  }

  struct RobotTasks tasks;
  MAKE_HEAD(tasks, ASSIGN_ROBOT_TASKS, 0, 0, 0, 0, 0,
            manager_info_->svr_info.type, manager_info_->svr_info.id, 0);
  int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_ROBOT_TASKS);
  LOG_DEBUG2("task_type[%d] map to crawler_type[%d]", ASSIGN_ROBOT_TASKS,
             crawler_type);
  if (!crawler_schduler_engine_->CheckOptimalRouter(crawler_type)) {
    LOG_MSG2("no have OptimalCrawler with crawler_type: %d", crawler_type);
    return true;
  }

  int32 count = task_cache_->task_idle_map_.size();
  int32 index = 0;

  TASKINFO_MAP::iterator it = task_cache_->task_idle_map_.begin();
  TASKINFO_MAP::iterator packet_start = it;

  for (; it != task_cache_->task_idle_map_.end(), index < count; index++) {
    base_logic::RobotTaskContent con;
    base_logic::LoginCookie cookie;
    base_logic::RobotTask *info = (it++)->second;

    if (info->create_time() + info->TTL < current_time) {
      LOG_DEBUG2("task[%lld] has time out, delete it, "
                 "create time: %lld, current time: %lld, ttl: %d",
                 info->id(), info->create_time(), current_time);
      info->set_state(TASK_INVALID);
      continue;
    }

    if (!info->IsReady(current_time)) {
      LOG_MSG2("task is not ready, id: %lld, type: %d",
               info->id(), (int)info->type());
      continue;
    }

    if (!content_cache_->GetContentByAttrId(info->type(), con)) {
      LOG_MSG2("task(id: %d, type: %d) has no content, ignore it", info->id(),
               info->type());
      continue;
    }

    if (!cookie_cache_->GetCookie(info->type(), cookie)) {
      LOG_MSG2("no cookie for type: %d", (int)info->type());
      continue;
    }

    info->set_cookie(cookie);
    info->set_content(con);

    struct RobotTaskBase *unit = info->CreateTaskPacketUnit();

    tasks.task_set.push_back(unit);
    info->set_state(TASK_SEND);
    info->set_send_time();
    task_db_->UpdateRobotTaskDetail(info);
    LOG_MSG2("DistributionTask task_type = %d, task_id=%d", info->type(),
             info->id());

    info->UpdateNextExecTime();

    if (tasks.task_set.size() % base_num == 0 && tasks.task_set.size() != 0) {
      tasks.task_num = tasks.task_set.size();
      bool send_success = true;
//			net::PacketProsess::DumpPacket(&tasks);
      send_success = crawler_schduler_engine_->SendOptimalRouter(
          (const void*) &tasks, 0, crawler_type);
      if (!send_success) {
        LOG_DEBUG2("packet_start ~ it distance: %d",
                   std::distance(packet_start, it));
        for (; packet_start != it; ++packet_start) {
          base_logic::RobotTask *task = packet_start->second;
          task->set_state(TASK_SEND_FAILED);
          task_db_->UpdateRobotTaskState(task->id(), (int) task->state(),
                                         std::string("none"));
        }
      }
      packet_start = it;
      net::PacketProsess::ClearRobotTaskList(&tasks);
      // 每次只分发一个包
      break;
    }
  }

  //解决余数
  if (tasks.task_set.size() > 0) {
    tasks.task_num = tasks.task_set.size();
    bool send_success = true;
//		net::PacketProsess::DumpPacket(&tasks);
    send_success = crawler_schduler_engine_->SendOptimalRouter(
        (const void*) &tasks, 0, crawler_type);
    if (!send_success) {
      LOG_DEBUG2("packet_start ~ it distance: %d",
                 std::distance(packet_start, it));
      for (; packet_start != it; ++packet_start) {
        base_logic::RobotTask *task = packet_start->second;
        task->set_state(TASK_SEND_FAILED);
        task_db_->UpdateRobotTaskState(task->id(), (int) task->state(),
                                       std::string("none"));
      }
    }
    net::PacketProsess::ClearRobotTaskList(&tasks);
  }

  //
  for (it = task_cache_->task_idle_map_.begin();
      it != task_cache_->task_idle_map_.end();) {
    base_logic::RobotTask *task = it->second;
    if (TASK_INVALID == task->state()) {
      task_cache_->task_idle_map_.erase(it++);
    } else if (TASK_SEND == task->state()) {
      task_cache_->task_exec_map_[task->id()] = task;
      task_cache_->task_idle_map_.erase(it++);
    } else {
      ++it;
    }
  }
  return true;
}

void TaskSchdulerManager::CheckIsEffective() {
  crawler_schduler_engine_->CheckIsEffective();
}

uint32 TaskSchdulerManager::GetExecTasks() {
  return task_cache_->task_exec_map_.size();
}

}  // namespace crawler_task_logic
