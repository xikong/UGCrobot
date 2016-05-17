/*
 * forgery_ip_engine.h
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_ROBOT_TASK_FORGERY_IP_ENGINE_H_
#define KID_PLUGINS_ROBOT_TASK_FORGERY_IP_ENGINE_H_
#include <map>
#include <list>
#include "basic/basictypes.h"

namespace base_logic {
class ForgeryIP;
}

namespace robot_task_logic {
class CrawlerTaskDB;
}

class ForgeryIPManager {
public:
	ForgeryIPManager();
	virtual ~ForgeryIPManager();

	bool Init(robot_task_logic::CrawlerTaskDB *task_db);

	// 如果 forgery_ip 的 id 不为空，则根据该 id 获取 ip
	// 否则根据默认算法获取 ip
	bool GetIP(base_logic::ForgeryIP &forgery_ip);
	bool SetIPs();
	bool Empty() const { return ip_cache_.ip_container.ip_list.empty(); }
private:
	typedef std::list<base_logic::ForgeryIP>	IPList;
	typedef std::list<base_logic::ForgeryIP>::const_iterator	IPListConstIt;
	struct IPContainer {
		IPList	ip_list;
	};

	typedef std::map<int64, base_logic::ForgeryIP>	IPIdMap;
	struct IPCache {
		IPIdMap		ip_id_map;
		IPContainer	ip_container;
	};
private:
	robot_task_logic::CrawlerTaskDB		*task_db_;
	IPCache								ip_cache_;
};

class ForgeryIPEngine {
 private:
    static ForgeryIPManager           *forgery_ip_mgr_;
    static ForgeryIPEngine            *forgery_ip_engine_;

    ForgeryIPEngine() {}
    virtual ~ForgeryIPEngine() {}
 public:
//    __attribute__((visibility("default")))
     static ForgeryIPManager* GetForgeryIPManager() {
        if (forgery_ip_mgr_ == NULL)
            forgery_ip_mgr_ = new ForgeryIPManager();
        return forgery_ip_mgr_;
    }

    static ForgeryIPEngine* GetForgeryIPEngine() {
        if (forgery_ip_engine_ == NULL)
            forgery_ip_engine_ = new ForgeryIPEngine();

        return forgery_ip_engine_;
    }
};

#endif /* KID_PLUGINS_ROBOT_TASK_FORGERY_IP_ENGINE_H_ */
