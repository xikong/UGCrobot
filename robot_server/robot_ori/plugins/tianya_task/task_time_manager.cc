//  Copyright (c) 2015-2015 The kid Authors. All rights reserved.
//  Created on: 2015年10月12日 Author: kerry

#include <list>
#include "task_time_manager.h"
#include "logic/logic_unit.h"

namespace tianya_task_logic {

TaskTimeManager::TaskTimeManager(tianya_task_logic::CrawlerTaskDB* task_db) {
    schduler_mgr_ =  tianya_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
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
        TimeFechTempTask();
        break;
      case TIME_DISTRBUTION_TEMP_TASK:
        schduler_mgr_->DistributionTempTask();
        break;
      case TIME_UPDATE_EXEC_TASKS:
    	UpdateExecTasks();
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
	schduler_mgr_->SetBatchCookies();
	schduler_mgr_->SetBatchContents();
    std::list<base_logic::TianyaTask> list;
    task_kafka_.FectchBatchTempTask(&list);
    //task_db_->BatchUpdateTaskInfo(&list);

    schduler_mgr_->FetchBatchTask(&list);
}


void TaskTimeManager::TimeFechTempTask() {
#if 0
    std::list<base_logic::WeiboTaskInfo> list;
	std::list<struct RobotCookie> cookie_list;
	std::list<struct RobotIP> ip_list;
    task_kafka_.FectchBatchTempTask(&list);
    //task_db_->BatchUpdateTaskInfo(&list);
	task_db_->FetchRobotCookies(&cookie_list, 1, list.size());
	task_db_->FetchRobotIP(&ip_list, list.size());
	std::list<base_logic::WeiboTaskInfo>::iterator task_it = list.begin();
	std::list<struct RobotCookie>::iterator cookie_it = cookie_list.begin();
	std::list<struct RobotIP>::iterator ip_it = ip_list.begin();
	for (; list.end() != task_it && 
			cookie_list.end() != cookie_it &&
			ip_list.end() != ip_list; 
			++task_it, ++cookie_it, ++ip_it) {
		task_it->set_cookie_id(cookie_it->id);
		task_it->set_cookie(cookie_it->cookie);
		task_it->set_addr(ip_it->ip);
	}
    schduler_mgr_->FetchBatchTemp(&list);
#endif
}

void TaskTimeManager::TimeCheckTask() {
    schduler_mgr_->RecyclingTask();
}

}  // namespace crawler_task_logic

