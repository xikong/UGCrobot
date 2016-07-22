/*
 * CookieManager.cpp
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#include "cookie_engine.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"
#include "forgery_ip_engine.h"
#include "forgery_ua_engine.h"
#include "robot_config.h"		// for Config

CookieManager* CookieEngine::cookie_mgr_ = NULL;
CookieEngine* CookieEngine::cookie_engine_ = NULL;

CookieManager::CookieManager() {
  // TODO Auto-generated constructor stub
  config_ = NULL;
  task_db_ = NULL;
  InitThreadMutex(&lock_, PTHREAD_MUTEX_RECURSIVE);
}

CookieManager::~CookieManager() {
  // TODO Auto-generated destructor stub
  DeinitThreadMutex (lock_);
}

bool CookieManager::Init(robot_task_logic::Config *config,
                         robot_task_logic::CrawlerTaskDB *task_db) {
  if (NULL == config || NULL == task_db) {
    LOG_MSG("cookie engine init fail");
    return false;
  }
  config_ = config;
  task_db_ = task_db;
  FetchCookies(0);
  return true;
}

bool CookieManager::HasAvailableCookie(int64 attr_id) {
  base_logic::MLockGd lk(lock_);
  CookieList &cookie_list =
      cookies_cache_.cookie_attr_id_map_[attr_id].cookie_list;
  time_t available_time = time(NULL) - config_->GetCookieTickByAttrId(attr_id);

  if (!cookie_list.empty()) {
    LOG_DEBUG2("attr_id: %lld, last_use_time: %lld, available_time: %lld",
               attr_id, cookie_list.front().last_use_time(), available_time);
  }
  return !cookie_list.empty()
      && cookie_list.front().last_use_time() < available_time;
}

bool CookieManager::WriteCookieUseTime(int64 attr_id) {
  if (NULL == task_db_) {
    return false;
  }
  base_logic::MLockGd lk(lock_);
  if (0 == attr_id) {  // 更新所有平台cookie 使用时间
    CookieIdMap &cookie_id_map = cookies_cache_.cookie_id_map_;
    CookieIdMap::const_iterator it = cookie_id_map.begin();
    for (; it != cookie_id_map.end(); ++it) {
      task_db_->UpdateCookieAccessTime(it->second.cookie_id(),
                                       it->second.last_use_time());
    }
  } else {
    CookieList &cookie_list =
        cookies_cache_.cookie_attr_id_map_[attr_id].cookie_list;
    CookieListConstIt it = cookie_list.begin();
    for (; it != cookie_list.end(); ++it) {
      task_db_->UpdateCookieAccessTime(it->cookie_id(), it->last_use_time());
    }
  }
}

void CookieManager::BindForgeryIP(CookieList &cookie_list) {
  if (cookie_list.empty()) {
    return;
  }
  // forgery_ip_engine 由 task_schedule_engine 初始化
  ForgeryIPManager *ip_manager = ForgeryIPEngine::GetForgeryIPManager();
  if (ip_manager->Empty()) {
    LOG_MSG("there has NONE ip to bind cookie");
    return;
  }
  CookieList::iterator it = cookie_list.begin();
  for (; it != cookie_list.end(); ++it) {
    if (0 == it->ip_.id()) {
      ip_manager->GetIP(it->ip_);
      task_db_->BindIPToCookie(it->cookie_id(), it->ip_.id(), it->ip_.ip());
    }
  }
}

void CookieManager::BindForgeryUA(CookieList &cookie_list) {
  if (cookie_list.empty()) {
    return;
  }
  // forgery_ua_engine 由 task_schedule_engine 初始化
  ForgeryUAManager *ua_manager = ForgeryUAEngine::GetForgeryUAManager();
  if (ua_manager->Empty()) {
    LOG_MSG("there has NONE ua to bind cookie");
    return;
  }

  CookieList::iterator it = cookie_list.begin();
  for (; it != cookie_list.end(); ++it) {
    if (0 != it->ua_.id() && it->ua_.ua().empty()) {
      ua_manager->GetUA(it->ua_);
    } else {
      ua_manager->GetUA(it->ua_);
      it->ua_.update_access_time();
      task_db_->BindUAToCookie(it->cookie_id(), it->ua_.id());
    }
  }
}

bool CookieManager::FetchCookies(int64 attr_id) {
  if (NULL == task_db_) {
    return false;
  }

  time_t current_time = time(NULL);
  time_t usable_time = uint64(-1);
  LOG_DEBUG2("usable_time = %lld", usable_time);

  //
  base_logic::MLockGd lk(lock_);
  WriteCookieUseTime(attr_id);

  CookieList cookie_list;
  if (!task_db_->GetCookies(0, attr_id, usable_time, &cookie_list)) {
    LOG_MSG("fecth cookie from mysql error");
    return false;
  }
  LOG_DEBUG2("fetch %d cookies with attr_id[%lld] from mysql",
             cookie_list.size(), attr_id);
  if (cookie_list.empty()) {
    LOG_MSG2("attr_id[%lld] has no more available cookie", attr_id);
    return false;
  }

  cookie_list.sort(base_logic::LoginCookie::cmp);
  BindForgeryIP(cookie_list);
  BindForgeryUA(cookie_list);

  cookies_cache_.Clear();
  CookieIdMap &cookie_id_map = cookies_cache_.cookie_id_map_;
  CookieAttrIdMap &attr_id_map = cookies_cache_.cookie_attr_id_map_;
  CookieListConstIt it = cookie_list.begin();
  for (; it != cookie_list.end(); ++it) {
    cookie_id_map[it->cookie_id()] = *it;
    attr_id_map[it->get_cookie_attr_id()].cookie_list.push_back(*it);
  }
  LOG_DEBUG2("cookid_id_map size: %d", cookie_id_map.size());
  return true;
}

bool CookieManager::GetCookie(int64 attr_id, base_logic::LoginCookie& cookie) {
  base_logic::MLockGd lk(lock_);
  if (!HasAvailableCookie(attr_id)) {
    return false;
  }
  CookieList &cookie_list =
      cookies_cache_.cookie_attr_id_map_[attr_id].cookie_list;
  cookie = cookie_list.front();
  cookie.update_use_time();
  cookie_list.pop_front();
  cookie_list.push_back(cookie);
  return true;
}

bool CookieManager::RemoveInvalidCookie(int64 cookie_id) {
  base_logic::MLockGd lk(lock_);
  CookieIdMap &cookie_id_map = cookies_cache_.cookie_id_map_;
  CookieIdMap::iterator it1 = cookie_id_map.find(cookie_id);
  if (cookie_id_map.end() == it1) {
    LOG_MSG2("don't find the cookie with cookie_id: %lld in cookie_id_map",
             cookie_id);
    return false;
  }
  int64 attr_id = it1->second.get_cookie_attr_id();
  cookie_id_map.erase(it1);

  CookieList &cookie_list = cookies_cache_.cookie_attr_id_map_[attr_id]
      .cookie_list;
  for (CookieList::iterator it2 = cookie_list.begin(); it2 != cookie_list.end();
      ++it2) {
    if (it2->cookie_id() == cookie_id) {
      cookie_list.erase(it2);
      return true;
    }
  }
  LOG_MSG2("don't find the cookie with cookie_id: %lld in cookie_list",
           cookie_id);
  return false;
}
