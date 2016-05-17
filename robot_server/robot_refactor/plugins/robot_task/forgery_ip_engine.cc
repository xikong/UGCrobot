/*
 * forgery_ip_engine.cpp
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#include "forgery_ip_engine.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"

ForgeryIPManager* ForgeryIPEngine::forgery_ip_mgr_ = NULL;
ForgeryIPEngine* ForgeryIPEngine::forgery_ip_engine_ = NULL;

ForgeryIPManager::ForgeryIPManager() {
	// TODO Auto-generated constructor stub
	task_db_ = NULL;
}

ForgeryIPManager::~ForgeryIPManager() {
	// TODO Auto-generated destructor stub
}

bool ForgeryIPManager::Init(robot_task_logic::CrawlerTaskDB *task_db) {
	if (NULL == task_db) {
		LOG_MSG("forgery ip engine init fail");
		return false;
	}
	task_db_ = task_db;
	return SetIPs();
}

bool ForgeryIPManager::SetIPs() {
	if (NULL == task_db_) {
		return false;
	}
	IPList &ip_list = ip_cache_.ip_container.ip_list;
	IPIdMap &id_map = ip_cache_.ip_id_map;

	ip_list.clear();
	id_map.clear();
	if (!task_db_->FectchBatchForgeryIP(&ip_list)) {
		LOG_MSG("fetch ip from myql error");
		return false;
	}
	for (IPListConstIt it = ip_list.begin(); it != ip_list.end(); ++it) {
		id_map[it->id()] = *it;
	}
	ip_list.sort(base_logic::ForgeryIP::cmp);
	LOG_DEBUG2("forgery ip size: %d", ip_list.size());
	return true;
}

bool ForgeryIPManager::GetIP(base_logic::ForgeryIP &forgery_ip) {
	IPList &ip_list = ip_cache_.ip_container.ip_list;
	IPList::iterator it = ip_list.begin();
	IPIdMap &id_map = ip_cache_.ip_id_map;
	if (ip_list.empty()) {
		LOG_MSG("there have NONE IPs");
		return false;
	}
	if (0 == forgery_ip.id()) { // 根据默认算法取 ip
		forgery_ip = *it;
	} else { // 根据 id 获取  ip
		for (it = ip_list.begin(); it != ip_list.end(); ++it) {
			if (it->id() == forgery_ip.id()) {
				forgery_ip = *it;
			}
		}
	}
	if (ip_list.end() == it) {
		return false;
	}
	LOG_DEBUG2("get ip: %s, last_use_time: %lld", forgery_ip.ip().c_str(),
			forgery_ip.access_time());
	it->update_access_time();
	ip_list.erase(it);
	ip_list.push_back(forgery_ip);
	return true;
}
