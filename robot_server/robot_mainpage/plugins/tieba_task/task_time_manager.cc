//  Copyright (c) 2015-2015 The kid Authors. All rights reserved.
//  Created on: 2015年10月12日 Author: kerry

#include <list>
#include "task_time_manager.h"
#include "logic/logic_unit.h"
#include "forgery_ip_engine.h"
#include "kw_blacklist.h"

namespace tieba_task_logic {

TaskTimeManager::TaskTimeManager(tieba_task_logic::CrawlerTaskDB* task_db) {
  schduler_mgr_ =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  ip_mgr_ = ForgeryIPEngine::GetForgeryIPManager();
  task_db_.reset(task_db);
  session_mgr_ = NULL;
}

TaskTimeManager::~TaskTimeManager() {
}

CrawlerTaskKafka& TaskTimeManager::GetTaskKafka() {
  return task_kafka_;
}

void TaskTimeManager::TaskTimeEvent(int opcode, int time) {
  if (NULL == session_mgr_) {
    LOG_MSG("wait server start up");
    return;
  }
  if (!session_mgr_->ServerIsValid()) {
    LOG_MSG("server has not registered success, don't process time event");
    return;
  }
  switch (opcode) {
    case TIME_DISTRIBUTION_TASK:
      schduler_mgr_->DistributionTask();
      break;
    case TIME_FECTCH_TASK:
      TimeFetchTask();
      break;
    case TIME_CLEAN_NO_EFFECTIVE:
      schduler_mgr_->CheckIsEffective();
      break;
    case TIME_RECYCLINGTASK:
      schduler_mgr_->RecyclingTask();
      break;
    case TIME_FETCH_MAIN_TASK:
      TimeFechMainTask();
      break;
    case TIME_DISTRBUTION_TEMP_TASK:
      schduler_mgr_->DistributionTempTask();
      break;
    case TIME_UPDATE_EXEC_TASKS:
      UpdateExecTasks();
      break;
    case TIME_FETCH_IP:
      ip_mgr_->FetchForgeryIPs();
      break;
    case TIME_FETCH_BLACK_KW:
      KwBlacklist::GetKwBlacklist()->FetchBlackKws(task_db_.get());
      break;
    default:
      break;
  }
}

void TaskTimeManager::UpdateExecTasks() {
  if (NULL == session_mgr_) {
    return ;
  }
  session_mgr_->GetServer()->set_busy_tasks(schduler_mgr_->GetExecTasks());
}

void TaskTimeManager::TimeFetchTask() {
  schduler_mgr_->SetBatchCookies();
  std::list<base_logic::TiebaTask> list;
  task_kafka_.FectchBatchTempTask(&list);
  //task_db_->BatchUpdateTaskInfo(&list);

  schduler_mgr_->FetchBatchTask(&list);
}

void TaskTimeManager::TimeFechMainTask() {
  std::list<base_logic::TiebaTask> list;
  task_db_->FecthBatchTask(&list);
  schduler_mgr_->FetchMainTask(&list);
}

void TaskTimeManager::TimeCheckTask() {
  schduler_mgr_->RecyclingTask();
}

}  // namespace crawler_task_logic

