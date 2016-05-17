/*
 * forgery_ua_engine.cpp
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#include "forgery_ua_engine.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"

ForgeryUAManager* ForgeryUAEngine::forgery_ua_mgr_ = NULL;
ForgeryUAEngine* ForgeryUAEngine::forgery_ua_engine_ = NULL;

ForgeryUAManager::ForgeryUAManager() {
	// TODO Auto-generated constructor stub
	task_db_ = NULL;
}

ForgeryUAManager::~ForgeryUAManager() {
	// TODO Auto-generated destructor stub
}

bool ForgeryUAManager::Init(robot_task_logic::CrawlerTaskDB *task_db) {
	if (NULL == task_db) {
		LOG_MSG("forgery ip engine init fail");
		return false;
	}
	task_db_ = task_db;
	return SetUAs();
}

bool ForgeryUAManager::SetUAs() {
	if (NULL == task_db_) {
		return false;
	}
	UAList &ua_list = ua_cache_.ua_container.ua_list;
	if (!task_db_->FectchBatchForgeryUA(&ua_list)) {
		LOG_MSG("fetch ua from myql error");
		return false;
	}
//	ua_list.sort(base_logic::ForgeryUA::cmp);
//	UAQueue &ua_queue = ua_cache_.ua_container.ua_queue;
//	for (UAListConstIt it = ua_list.begin(); ua_list.end() != it; ++it) {
//		ua_queue.push(*it);
//	}
	LOG_DEBUG2("forgery ua size: %d", ua_list.size());
	return true;
}

bool ForgeryUAManager::GetUA(base_logic::ForgeryUA &forgery_ua) {
	UAList &ua_list = ua_cache_.ua_container.ua_list;
	if (ua_list.empty()) {
		LOG_MSG("there have NONE UAs");
		return false;
	}
	UAList::iterator it = ua_list.begin();
	if (0 != forgery_ua.id()) {
		for (; it != ua_list.end(); ++it) {
			if (it->id() == forgery_ua.id()) {
				break;
			}
		}
	}
	if (ua_list.end() == it) {
		LOG_MSG2("don't find the ua with id: %d", forgery_ua.id());
		return false;
	}
	forgery_ua = *it;
	LOG_DEBUG2("get ua: %s, last_access_time: %lld", forgery_ua.ua().c_str(),
			forgery_ua.access_time());
	it->update_access_time();
	ua_list.erase(it);
	ua_list.push_back(forgery_ua);
	return true;
}
