//  Copyright (c) 2015-2015 The kid Authors. All rights reserved.
//  Created on: 2015年10月12日 Author: kerry

#include <list>
#include "task_time_manager.h"
#include "logic/logic_unit.h"
#include "cookie_engine.h"
#include "forgery_ip_engine.h"

namespace robot_task_logic {

TaskTimeManager::TaskTimeManager(robot_task_logic::CrawlerTaskDB* task_db) {
    schduler_mgr_ =  robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
    cookie_mgr_ = CookieEngine::GetCookieManager();
    forgery_ip_mgr_ = ForgeryIPEngine::GetForgeryIPManager();
    task_db_.reset(task_db);
    manager_info_ = NULL;
}

TaskTimeManager::~TaskTimeManager() {
}

void TaskTimeManager::TaskTimeEvent(int opcode, int time) {
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
      case TIME_FETCH_TEMP_TASK:
//        TimeFechTempTask();
        break;
      case TIME_DISTRBUTION_TEMP_TASK:
//        schduler_mgr_->DistributionTempTask();
        break;
      case TIME_UPDATE_EXEC_TASKS:
    	UpdateExecTasks();
        break;
      case TIME_FETCH_IP:
    	  forgery_ip_mgr_->FetchForgeryIPs();
    	  break;
      case TIME_WRITE_COOKIE_USE_TIME:
    	  cookie_mgr_->WriteCookieUseTime(0);
    	  break;
//      case TIME_FETCH_CONTENT:
//    	  schduler_mgr_->SetBatchContents();
//    	  break;
      case TIME_FETCH_COOKIES:
    	  cookie_mgr_->FetchCookies(0);
    	  break;
      default:
        break;
    }
}

void TaskTimeManager::UpdateExecTasks() {
	if (NULL == manager_info_) {
		struct server *pserver = logic::CoreSoUtils::GetSRV();
		assert(pserver);
		void **p_share = pserver->get_plugin_share_data(pserver, "manager");
		assert(p_share);
		manager_info_ = (plugin_share::ManagerInfo *) *p_share;
		assert(manager_info_);
	}
	base_logic::MLockGd lk(manager_info_->lock_);
	manager_info_->svr_info.exec_tasks = schduler_mgr_->GetExecTasks();
}

void TaskTimeManager::TimeFetchTask() {
    std::list<base_logic::RobotTask *> list;
    task_kafka_.FectchTasks(&list);
    task_db_->RecordRobotTasks(list);
    schduler_mgr_->FetchBatchTask(&list);
}

void TaskTimeManager::TimeCheckTask() {
    schduler_mgr_->RecyclingTask();
}

}  // namespace crawler_task_logic

