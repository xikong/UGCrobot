//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月22日 Author: kerry

#ifndef KID_CRAWLER_TASK_SCHDULER_ENGINE_H_
#define KID_CRAWLER_TASK_SCHDULER_ENGINE_H_

#include <map>
#include <list>

#include "../../pub/logic/auto_crawler_infos0505.h"
#include "crawler_schduler/crawler_schduler_engine.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "config/config.h"

#include "../../pub/share/manager_info.h"
#include "crawler_task_db.h"

#define GET_COOKIES_PER_TIME	3000

namespace tieba_task_logic {

typedef std::map<int64, base_logic::TiebaTask> TASKINFO_MAP;
typedef std::list<base_logic::TiebaTask>  TASKINFO_LIST;

class TaskSchdulerCache {
 public:
    TASKINFO_MAP          task_idle_map_;
    TASKINFO_MAP          task_exec_map_;
    TASKINFO_LIST         task_temp_list_;
};

typedef std::list<base_logic::LoginCookie> CookieList;

struct CookiePlatform {
    CookieList	list;
    CookieList::iterator	cur_it;
    time_t      update_time_;
    CookiePlatform() {
    	cur_it = list.end();
    	update_time_ = 0;
    }
};

typedef std::map<int64, CookiePlatform> CookieMap;

class CookieCache {
 public:
	bool GetCookie(int64 attr_id, base_logic::LoginCookie &cookie) {
		time_t current_time = time(NULL);
		if (cookie_map_.end() == cookie_map_.find(attr_id)) {
			LOG_MSG2("can't find the cookie with the attr_id: %d", attr_id);
			return false;
		}
		struct CookiePlatform &platform = cookie_map_[attr_id];
		if (platform.list.end() == platform.cur_it)
			platform.cur_it = platform.list.begin();
		cookie = *platform.cur_it;
		if ((cookie.send_last_time() + 1800) > current_time) {
			LOG_MSG("cookie use to often");
			return false;
		}
		++platform.cur_it;
		cookie.update_time();
		return true;
	}

	void SortCookies() {
		CookieMap::iterator it = cookie_map_.begin();
		for (; cookie_map_.end() != it; ++it) {
			struct CookiePlatform &platform = it->second;
			platform.list.sort(base_logic::LoginCookie::cmp);
			platform.cur_it = platform.list.begin();
		}
	}
	public:
		CookieMap cookie_map_;
		std::map<int64, int64> update_time_map_;
		uint64 last_time;
		CookieCache()
		{
			last_time = 0;
		}
};

typedef std::map<std::string, int>	IpUseTimesMap;
class IPCache {
public:
	IPCache() {
		cur_it_ = ip_list_.end();
	}
	bool GetIP(base_logic::ForgeryIP &ip) {
		if (ip_list_.empty()) {
			LOG_MSG("there are no more IPs");
			return false;
		}
		ip = *ip_list_.begin();
		int use_times = ++ip_use_times_map_[ip.ip()];
		if (IP_USE_TIMES_LIMIT == use_times) {
			ip_list_.erase(ip_list_.begin());
			LOG_MSG2("ip: [%s] has use more than %d IP_USE_TIMES_LIMIT times, erase it, available ip: %d",
					ip.ip().c_str(), IP_USE_TIMES_LIMIT, ip_list_.size());
		}
		return true;
	}

	void Update(const std::list<base_logic::ForgeryIP> &ip_list) {
		std::list<base_logic::ForgeryIP>::const_iterator it = ip_list.begin();
		for (; it != ip_list.end(); ++it) {
			std::string ip = it->ip();
			if (ip_use_times_map_.end() == ip_use_times_map_.find(ip)) {
				ip_use_times_map_[ip] = 0;
				ip_list_.push_back(*it);
				LOG_DEBUG2("add new ip: %s", ip.c_str());
			}
		}
	}

	void Reset() {
		ip_list_.clear();
		IpUseTimesMap::iterator it = ip_use_times_map_.begin();
		for (; it != ip_use_times_map_.end(); ++it) {
			base_logic::ForgeryIP forgery_ip;
			forgery_ip.set_ip(it->first);
			ip_list_.push_back(forgery_ip);
		}
	}

	void SortIPBySendTime() {
		ip_list_.sort(base_logic::ForgeryIP::cmp);
		cur_it_ = ip_list_.begin();
	}
private:
	friend class TaskSchdulerManager;
	friend class CookieIpEngine;
	friend class CookieIpEngine;

	static const int IP_USE_TIMES_LIMIT = 4;
	std::list<base_logic::ForgeryIP>	   ip_list_;		//记录可用的 IP
	std::list<base_logic::ForgeryIP>::iterator cur_it_;
	IpUseTimesMap ip_use_times_map_;						//保存了所有IP
};

typedef std::map<std::string, IPCache> UserIpMap;
typedef std::list<std::string>  IpList;

class CookieIpEngine {
public:
	bool GetIpByCookie(const base_logic::LoginCookie &cookie, std::string &ip) {
		bool ret = true;
		UserIpMap::iterator it = user_ip_map_.find(cookie.get_username());
		if (user_ip_map_.end() == it) {
			LOG_MSG2("don't find the username: %s", cookie.get_username());
			return false;
		}
		IPCache &ip_cache = it->second;
		base_logic::ForgeryIP forgery_ip;
		if (!ip_cache.GetIP(forgery_ip)) {
			LOG_MSG2("assign ip to user: %s error", cookie.get_username());
			ret = false;
		}
		ip = forgery_ip.ip();
		return ret;
	}
	void Update(const std::list<base_logic::LoginCookie> &cookie_list,
			const std::list<base_logic::ForgeryIP> &ip_list) {
		std::string username;
		std::list<base_logic::LoginCookie>::const_iterator cookie_it = cookie_list.begin();
		for (; cookie_it != cookie_list.end(); ++cookie_it) {
			username = cookie_it->get_username();
			if (user_ip_map_.end() == user_ip_map_.find(username)) {
				LOG_MSG2("add new user: %s", username.c_str());
			}
			IPCache &ip_cache = user_ip_map_[username];
			ip_cache.Update(ip_list);
			LOG_DEBUG2("user: %s available ip: %d", username.c_str(), ip_cache.ip_list_.size());
		}
	}
	void Update(const std::list<base_logic::ForgeryIP> &ip_list) {
		UserIpMap::iterator it = user_ip_map_.begin();
		for (; it != user_ip_map_.end(); ++it) {
			IPCache &ip_cache = it->second;
			ip_cache.Update(ip_list);
			LOG_DEBUG2("user: %s available ip: %d", it->first.c_str(), ip_cache.ip_list_.size());
		}
	}
private:
	friend class TaskSchdulerManager;
	UserIpMap user_ip_map_;
};


typedef std::list<base_logic::RobotTaskContent>		TaskContentList;
struct TaskContent {
	TaskContent(): task_type(0), cur_it(content_list.end()) {}
	int16			task_type;
	TaskContentList	content_list;
	TaskContentList::iterator cur_it;
};

typedef std::map<int16, struct TaskContent>	TaskContentMap;
class TaskContentCache {
public:
	bool GetContentByTaskType(int16 task_type, std::string &content) {
		if (content_map_.end() == content_map_.find(task_type)) {
			LOG_MSG2("don't find content with task_type: %d", task_type);
			return false;
		}
		struct TaskContent &con = content_map_[task_type];
		LOG_DEBUG2("task_type: %d, content_list.size = %d",
				task_type, con.content_list.size());
		if (con.content_list.size() <= 0) {
			return false;
		}
		if (con.content_list.end() == con.cur_it) {
			con.cur_it = con.content_list.begin();
		}
		content = con.cur_it->content();
		++con.cur_it;
		return true;
	}
private:
	friend class TaskSchdulerManager;
	TaskContentMap	content_map_;
};

class TaskSchdulerManager {
 public:
    TaskSchdulerManager();
    virtual ~TaskSchdulerManager();

    void Init(router_schduler::SchdulerEngine* crawler_engine);

    void InitDB(tieba_task_logic::CrawlerTaskDB*     task_db);

    void InitManagerInfo(plugin_share::ManagerInfo *info);

    void FetchBatchTask(std::list<base_logic::TiebaTask> *list,
            bool is_first = false);

    void FetchBatchTemp(std::list<base_logic::TiebaTask>* list);

 public:
    bool DistributionTask();

    void RecyclingTask();

    bool AlterTaskState(const int64 task_id, const int8 state);

    bool AlterCrawlNum(const int64 task_id, const int64 num);

    void CheckIsEffective();

    uint32 GetExecTasks();

    int64& GetDatabaseUpdateTimeByPlatId(const int64 plat_id);

    void SetBatchCookies();

    void SetBatchContents();

    void SetBatchIP();
 private:
    void Init();

    void SetContent(const base_logic::RobotTaskContent &con);

    void SetCookie(const base_logic::LoginCookie& info);

    void CheckBatchCookie(const int64 plat_id);

    void SetBatchCookie(const int64 plat_id, const int64 from);

    bool FectchBacthCookies(const int64 plat_id, const int64 count,
            std::list<base_logic::LoginCookie>* list);

    void FecthAndSortCookies(const int64 count,
            std::list<base_logic::LoginCookie>& src_list,
            std::list<base_logic::LoginCookie>* dst_list, int64 plat_id);

    void SetUpdateTime(const int64 plat_id, const int64 update_time);

    void PrintInfo();


 private:
    struct threadrw_t*                     lock_;
    plugin_share::ManagerInfo			   *manager_info_;
    TaskSchdulerCache*                     task_cache_;
    router_schduler::SchdulerEngine*       crawler_schduler_engine_;
    int32                                  crawler_count_;
    tieba_task_logic::CrawlerTaskDB*       task_db_;
	CookieCache							   *cookie_cache_;
	IPCache								   *ip_cache_;
	TaskContentCache					   *content_cache_;
	CookieIpEngine						   *cookie_ip_manager_;
};

class TaskSchdulerEngine {
 private:
    static TaskSchdulerManager    *schduler_mgr_;
    static TaskSchdulerEngine     *schduler_engine_;

    TaskSchdulerEngine() {}
    virtual ~TaskSchdulerEngine() {}
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
