/*
 * CookieManager.h
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_ROBOT_TASK_COOKIE_ENGINE_H_
#define KID_PLUGINS_ROBOT_TASK_COOKIE_ENGINE_H_

#include <map>
#include <list>
#include "basic/basictypes.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"

namespace base_logic {
class LoginCookie;
}

namespace robot_task_logic {
class Config;
class CrawlerTaskDB;
}

class CookieManager {
 public:
  typedef std::list<base_logic::LoginCookie> CookieList;

  CookieManager();
  virtual ~CookieManager();

  bool Init(robot_task_logic::Config *config,
            robot_task_logic::CrawlerTaskDB *task_db);
  bool FetchCookies(int64 attr_id);
  bool GetCookie(int64 attr_id, base_logic::LoginCookie&);
  bool RemoveInvalidCookie(int64 cookie_id);
  void BindForgeryIP(CookieList &cookie_list);
  void BindForgeryUA(CookieList &cookie_list);
  bool HasAvailableCookie(int64 attr_id);

  // 如果 attr_id 为0，则更新所有平台 cookie
  bool WriteCookieUseTime(int64 attr_id);
 private:
  typedef CookieList::const_iterator CookieListConstIt;
  struct CookieContainer {
    CookieList cookie_list;
  };

  typedef std::map<int64, base_logic::LoginCookie> CookieIdMap;
  typedef std::map<int64, CookieContainer> CookieAttrIdMap;
  struct CookiesCache {
    CookieIdMap cookie_id_map_;
    CookieAttrIdMap cookie_attr_id_map_;
    void Clear() {
      cookie_id_map_.clear();
      cookie_attr_id_map_.clear();
    }
  };

 private:
  static const int COOKIE_CACHE_SIZE = 3000;
  struct threadmutex_t *lock_;
  robot_task_logic::Config *config_;
  robot_task_logic::CrawlerTaskDB *task_db_;
  CookiesCache cookies_cache_;
};

class CookieEngine {
 private:
  static CookieManager *cookie_mgr_;
  static CookieEngine *cookie_engine_;

  CookieEngine() {
  }
  virtual ~CookieEngine() {
  }
 public:
//    __attribute__((visibility("default")))
  static CookieManager* GetCookieManager() {
    if (cookie_mgr_ == NULL)
      cookie_mgr_ = new CookieManager();
    return cookie_mgr_;
  }

  static CookieEngine* GetCookieEngine() {
    if (cookie_engine_ == NULL)
      cookie_engine_ = new CookieEngine();

    return cookie_engine_;
  }
};

#endif /* KID_PLUGINS_ROBOT_TASK_COOKIE_ENGINE_H_ */
