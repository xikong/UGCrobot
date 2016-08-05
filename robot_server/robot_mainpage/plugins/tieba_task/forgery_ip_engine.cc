/*
 * forgery_ip_engine.cpp
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#include "forgery_ip_engine.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"
#include "basic/radom_in.h"

ForgeryIPManager* ForgeryIPEngine::forgery_ip_mgr_ = NULL;
ForgeryIPEngine* ForgeryIPEngine::forgery_ip_engine_ = NULL;

ForgeryIPManager::ForgeryIPManager() {
  // TODO Auto-generated constructor stub
  task_db_ = NULL;
  InitThreadrw(&lock_);
}

ForgeryIPManager::~ForgeryIPManager() {
  // TODO Auto-generated destructor stub
  DeinitThreadrw(lock_);
}

bool ForgeryIPManager::Init(CrawlerTaskDB *task_db) {
  if (NULL == task_db) {
    LOG_MSG("forgery ip engine init fail");
    return false;
  }
  task_db_ = task_db;
  return FetchForgeryIPs();
}

bool ForgeryIPManager::FetchForgeryIPs() {
  if (NULL == task_db_) {
    return false;
  }
  base_logic::WLockGd lk(lock_);
  ip_cache_.Clear();
  IPList &ip_list = ip_cache_.ip_list;

  if (!task_db_->FectchBatchForgeryIP(&ip_list)) {
    LOG_MSG("fetch ip from myql error");
    return false;
  }

  IPList::iterator it = ip_list.begin();
  IPIdMap &id_map = ip_cache_.ip_id_map;
  IPAttrIdMap &attr_id_map = ip_cache_.ip_attr_id_map;
  for (; it != ip_list.end(); ++it) {
    id_map[it->id()] = *it;
    attr_id_map[it->attr_id()].push_back(*it);
  }
  LOG_DEBUG2("forgery ip size: %d", ip_list.size());
  return true;
}

base_logic::ForgeryIP ForgeryIPManager::RandomIP() {
  IPList& ip_list = ip_cache_.ip_list;
  if (ip_list.empty()) {
    return base_logic::ForgeryIP();
  }
  uint32 i = 0;
  std::size_t size = ip_list.size();
  IPList::const_iterator cit = ip_list.begin();
  uint32 random_i = base::SysRadom::GetInstance()->GetRandomIntID() % size;
  while (i++ < random_i && ++cit != ip_list.end());
  return *cit;
}

bool ForgeryIPManager::GetIPByAttrId(int64 attr_id, base_logic::ForgeryIP &forgery_ip) {
  if (base_logic::RobotTask::SNOWBALL == attr_id) {
    return true;
  }
  base_logic::WLockGd lk(lock_);
  IPAttrIdMap &attr_id_map = ip_cache_.ip_attr_id_map;
  IPAttrIdMap::iterator it = attr_id_map.find(attr_id);
  if (attr_id_map.end() == it) {
    forgery_ip = RandomIP();
    return false;
  }
  IPList &ip_list = it->second;
  forgery_ip = ip_list.front();
  ip_list.pop_front();
  ip_list.push_back(forgery_ip);

  LOG_DEBUG2("get ip: %s with attr_id: %lld",
             forgery_ip.ip().c_str(), attr_id);
  return true;
}
