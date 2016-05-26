/*
 * forgery_ua_engine.h
 *
 *  Created on: 2016年5月16日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_ROBOT_TASK_FORGERY_UA_ENGINE_H_
#define KID_PLUGINS_ROBOT_TASK_FORGERY_UA_ENGINE_H_

#include <map>
#include <list>
#include <queue>

namespace base_logic {
class ForgeryUA;
}

namespace robot_task_logic {
class CrawlerTaskDB;
}

class ForgeryUAManager {
public:
	ForgeryUAManager();
	virtual ~ForgeryUAManager();
public:
	bool Init(robot_task_logic::CrawlerTaskDB *task_db);
	bool FetchForgeryUAs();

	// 如果 forgery_ua 的 id 为 0, 则按默认算法分配 ua
	// 否则分配指定 id 的 ua
	bool GetUA(base_logic::ForgeryUA &forgery_ua);
	bool Empty() const { return ua_cache_.ua_container.ua_list.empty(); }
private:
	typedef std::queue<base_logic::ForgeryUA>		UAQueue;
	typedef std::list<base_logic::ForgeryUA>		UAList;
	typedef UAList::const_iterator					UAListConstIt;
	struct UAContainer {
		UAList		ua_list;
	};
	struct UACache {
		UAContainer	ua_container;
	};
private:
	robot_task_logic::CrawlerTaskDB		*task_db_;
	UACache								ua_cache_;
};

class ForgeryUAEngine {
 private:
    static ForgeryUAManager           *forgery_ua_mgr_;
    static ForgeryUAEngine            *forgery_ua_engine_;

    ForgeryUAEngine() {}
    virtual ~ForgeryUAEngine() {}
 public:
//    __attribute__((visibility("default")))
     static ForgeryUAManager* GetForgeryUAManager() {
        if (forgery_ua_mgr_ == NULL)
            forgery_ua_mgr_ = new ForgeryUAManager();
        return forgery_ua_mgr_;
    }

    static ForgeryUAEngine* GetForgeryUAEngine() {
        if (forgery_ua_engine_ == NULL)
            forgery_ua_engine_ = new ForgeryUAEngine();

        return forgery_ua_engine_;
    }
};

#endif /* KID_PLUGINS_ROBOT_TASK_FORGERY_UA_ENGINE_H_ */
