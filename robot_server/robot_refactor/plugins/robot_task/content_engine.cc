/*
 * content_engine.cc
 *
 *  Created on: 2016年5月19日
 *      Author: zjc
 */

#include "content_engine.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"
#include "crawler_task_logic.h"		// for Config

ContentManager* ContentEngine::content_mgr_ = NULL;
ContentEngine* ContentEngine::content_engine_ = NULL;

ContentManager::ContentManager() {
	// TODO Auto-generated constructor stub
	config_ = NULL;
	task_db_ = NULL;
}

ContentManager::~ContentManager() {
	// TODO Auto-generated destructor stub
}

bool ContentManager::Init(Config *config, CrawlerTaskDB *task_db) {
	config_ = config;
	task_db_ = task_db;
	return NULL != task_db_;
}

bool ContentManager::HasContents(int64 attr_id) {
	return !cache_.content_attr_id_map[attr_id].content_queue.empty();
}

bool ContentManager::FetchContents(int64 attr_id) {
	if (NULL == task_db_) {
		return false;
	}
	ContentList content_list;
	if (!task_db_->FetchBatchTaskContent(attr_id, &content_list)) {
		LOG_MSG2("fetch contents fail with attr_id: %lld", attr_id);
		return false;
	}

	LOG_DEBUG2("fetch %d contents with attr_id: %lld",
			content_list.size(), attr_id);
	if (content_list.empty()) {
		return false;
	}

	ContentIdMap &id_map = cache_.content_id_map;
	ContentList::const_iterator it = content_list.begin();
	for (; it != content_list.end(); ++it) {
	  ContentQueue &content_queue = cache_.content_attr_id_map[it->task_type()].content_queue;
		id_map[it->id()] = *it;
		content_queue.push(*it);
	}
	return true;
}

bool ContentManager::GetContentByAttrId(int64 attr_id, RobotTaskContent &content) {
	if (!HasContents(attr_id) && !FetchContents(attr_id)) {
		LOG_MSG2("there has NONE %lld contents", attr_id);
		return false;
	}
	ContentQueue &content_queue = cache_.content_attr_id_map[attr_id].content_queue;
	content = content_queue.front();
	content_queue.pop();
	LOG_DEBUG2("content_queue size: %d", content_queue.size());
	return true;
}
