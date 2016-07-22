/*
 * content_engine.h
 *
 *  Created on: 2016年5月19日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_ROBOT_TASK_CONTENT_ENGINE_H_
#define KID_PLUGINS_ROBOT_TASK_CONTENT_ENGINE_H_

#include <map>
#include <queue>
#include <list>

#include "basic/basictypes.h"

namespace base_logic {
class RobotTaskContent;
}

namespace robot_task_logic {
class Config;
class CrawlerTaskDB;
}

using base_logic::RobotTaskContent;
using robot_task_logic::Config;
using robot_task_logic::CrawlerTaskDB;

class ContentManager {
public:
	typedef std::queue<RobotTaskContent>				ContentQueue;
	typedef std::list<RobotTaskContent>					ContentList;

	ContentManager();
	virtual ~ContentManager();
	bool Init(Config *config, CrawlerTaskDB *task_db);
	bool FetchContents(int64 attr_id);
	bool HasContents(int64 attr_id);
	bool GetContentByAttrId(int64 attr_id, RobotTaskContent &content);
private:

	struct ContentContainer {
		ContentQueue	content_queue;
	};
	typedef std::map<int64, RobotTaskContent>			ContentIdMap;
	typedef std::map<int64, ContentContainer>			ContentAttrIdMap;
	struct ContentCache {
		ContentIdMap		content_id_map;
		ContentAttrIdMap	content_attr_id_map;
		void Clear() {
			content_id_map.clear();
			content_attr_id_map.clear();
		}
	};
private:
	CrawlerTaskDB	*task_db_;
	Config			*config_;
	ContentCache	cache_;
};

class ContentEngine {
 private:
    static ContentManager           *content_mgr_;
    static ContentEngine            *content_engine_;

    ContentEngine() {}
    virtual ~ContentEngine() {}
 public:
//    __attribute__((visibility("default")))
     static ContentManager* GetContentManager() {
        if (content_mgr_ == NULL)
        	content_mgr_ = new ContentManager();
        return content_mgr_;
    }

    static ContentEngine* GetContentEngine() {
        if (content_engine_ == NULL)
        	content_engine_ = new ContentEngine();

        return content_engine_;
    }
};

#endif /* KID_PLUGINS_ROBOT_TASK_CONTENT_ENGINE_H_ */
