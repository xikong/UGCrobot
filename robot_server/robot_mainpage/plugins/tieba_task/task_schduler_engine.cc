//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry

#include <string>
#include <sstream>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"
#include "forgery_ip_engine.h"

namespace tieba_task_logic {

using namespace base_logic;

TaskSchdulerManager* TaskSchdulerEngine::schduler_mgr_ = NULL;
TaskSchdulerEngine* TaskSchdulerEngine::schduler_engine_ = NULL;

TaskSchdulerManager::TaskSchdulerManager() :
		crawler_count_(0), task_db_(NULL), session_mgr_(NULL) {
	task_cache_ = new TaskSchdulerCache();
	cookie_cache_ = new CookieCache();
	ip_cache_ = ForgeryIPEngine::GetForgeryIPManager();
	ua_cache_ = new ForgeryUACache();
	kw_blacklist_ = KwBlacklist::GetKwBlacklist();
	Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
	DeinitThreadrw(lock_);
  base::SysRadom::GetInstance()->DeinitRandom();
}

void TaskSchdulerManager::Init() {
	InitThreadrw(&lock_);
  base::SysRadom::GetInstance()->InitRandom();
}

void TaskSchdulerManager::InitDB(tieba_task_logic::CrawlerTaskDB* task_db) {
	task_db_ = task_db;
	ip_cache_->Init(task_db);
	task_db_->FectchBatchForgeryUA(&ua_cache_->ua_list_);
	LOG_MSG2("forgery ua size: %d", ua_cache_->ua_list_.size());
	SetBatchCookies();

  std::list<base_logic::TiebaTask> list;
  task_db_->FecthBatchTask(&list);
  FetchMainTask(&list);
}

void TaskSchdulerManager::Init(
		router_schduler::SchdulerEngine* crawler_engine) {
	crawler_schduler_engine_ = crawler_engine;
}

void TaskSchdulerManager::InitManagerInfo(plugin_share::SessionManager *session_mgr) {
	session_mgr_ = session_mgr;
}

void TaskSchdulerManager::SetBatchCookies() {
	std::list<base_logic::LoginCookie> list;
	base_logic::WLockGd lk(lock_);
	task_db_->GetCookies(GET_COOKIES_PER_TIME, 0, &list);
	while (list.size() > 0) {
		base_logic::LoginCookie info = list.front();
		list.pop_front();
		SetCookie(info);
	}
}

void TaskSchdulerManager::SetCookie(const base_logic::LoginCookie& info) {
	CookiePlatform& platform =
			cookie_cache_->cookie_map_[info.get_cookie_attr_id()];
	CookieList& list = platform.list;
	for (CookieList::iterator it = list.begin(); it != list.end();) {
		if (info.get_username() == it->get_username()
				&& info.get_passwd() == it->get_passwd()) {
//			LOG_MSG2("erase old cookie username=%s passwd=%s",
//					info.get_username().c_str(), info.get_passwd());
			list.erase(it++);
			break;
		} else {
			++it;
		}
	}
	platform.list.push_back(info);
	int64 info_update_time = info.get_update_time();
	platform.update_time_ = info_update_time;
	int64& plat_update_time = GetDatabaseUpdateTimeByPlatId(
			info.get_cookie_attr_id());
	if (plat_update_time < info_update_time)
		plat_update_time = info_update_time;

	if (info_update_time > cookie_cache_->last_time)
		cookie_cache_->last_time = plat_update_time;
}

bool IsMainTask(base_logic::TiebaTask &task) {
	if (task.is_main_task()) {
		return true;
	}
	return false;
}

void TaskSchdulerManager::FetchBatchTask(std::list<base_logic::TiebaTask> *list,
		bool is_first) {
	base_logic::WLockGd lk(lock_);

//    cookie_cache_->SortCookies();
	ua_cache_->SortUABySendTime();

	time_t current_time = time(NULL);
	while ((*list).size() > 0) {
		base_logic::TiebaTask info = (*list).front();
		(*list).pop_front();
		if (info.create_time()+30*60 < current_time) {
			LOG_DEBUG2("task timeout, url = %s, current_time = %lld, create_time = %lld",
					info.url().c_str(), current_time, info.create_time());
			continue;
		}

		if (kw_blacklist_->IsBlackKw(info.url())) {
		  LOG_MSG2("%s is in kw black list", info.url().c_str());
		  continue;
		}

		base_logic::ForgeryIP ip;
		base_logic::ForgeryUA ua;
		ip_cache_->GetIPByAttrId(info.type(), ip);
		ua_cache_->GetUA(ua);
		info.set_addr(ip.ip());
		info.set_ua(ua.ua());

		info.update_time(current_time,
				base::SysRadom::GetInstance()->GetRandomID(), is_first);
		info.set_id(base::SysRadom::GetInstance()->GetRandomID());

		TaskList &list = task_cache_->pending_task_map_[info.type()];
		list.attr_id_ = info.type();
		list.push_back(info);
	}
}

void TaskSchdulerManager::FetchMainTask(
		std::list<base_logic::TiebaTask>* list) {
  base_logic::WLockGd lk(lock_);

  ua_cache_->SortUABySendTime();
  task_cache_->ClearMainTask();
  time_t current_time = time(NULL);
  while ((*list).size() > 0) {
    base_logic::TiebaTask info = (*list).front();
    (*list).pop_front();
    base_logic::ForgeryIP ip;
    base_logic::ForgeryUA ua;
    ip_cache_->GetIPByAttrId(info.type(), ip);
    ua_cache_->GetUA(ua);
    info.set_addr(ip.ip());
    info.set_ua(ua.ua());
    TaskList &list = task_cache_->pending_task_map_[info.type()];
    list.attr_id_ = info.type();
    list.push_main_task(info);
  }
}

bool TaskSchdulerManager::AlterTaskState(const int64 task_id, const int state) {
	base_logic::WLockGd lk(lock_);
	task_cache_->task_exec_map_[task_id].set_state(state);
	if (TASK_SUCCESS == state || TASK_FAIL == state)
		task_cache_->task_exec_map_.erase(task_id);
	LOG_DEBUG2("task_exec_map size = %d", task_cache_->task_exec_map_.size());
	return true;
}

bool TaskSchdulerManager::AlterCrawlNum(const int64 task_id, const int64 num) {
	base_logic::WLockGd lk(lock_);
	/*
	 base_logic::TaskInfo task;
	 bool r = base::MapGet<TASKINFO_MAP, TASKINFO_MAP::iterator,
	 int64, base_logic::TaskInfo>(task_cache_->task_exec_map_,
	 task_id, task);
	 if (r)
	 task.set_crawl_num(num);
	 return r;
	 */
}

void TaskSchdulerManager::RecyclingTask() {
	base_logic::WLockGd lk(lock_);
	TASKINFO_MAP::iterator it = task_cache_->task_exec_map_.begin();
	time_t current_time = time(NULL);
	for (; it != task_cache_->task_exec_map_.end();) {
		base_logic::TiebaTask &task = it->second;
//        task_db_->RecordTaskState(task, 2);

		if ((task.send_time() + 60 * 60) < current_time) {
			task.set_state(TASK_WAIT);
			task_db_->UpdateRobotTaskState(task.id(), (int) TASK_TIMEOUT);
			LOG_DEBUG2("task[%d] didn't respond more than 5 min, erase it, "
					"send_time = %d, current_time = %d",
					task.id(), task.send_time(), current_time);
			task_cache_->task_exec_map_.erase(it++);
		} else {
			it++;
		}
	} LOG_DEBUG2("task_exec_map size = %d", task_cache_->task_exec_map_.size());
}

bool TaskSchdulerManager::DistributionTask() {
  if (NULL == session_mgr_) {
    LOG_MSG("wait server start up");
    return false;
  }
  base_logic::WLockGd lk(lock_);
  plugin_share::ServerSession *srv_session = session_mgr_->GetServer();
  int32 base_num = 1;
  int data_length = 0;
  time_t current_time = time(NULL);
  LOG_DEBUG2("distrubute tieba task current_time=%d task_cache_size=%d",
      (int)current_time, task_cache_->size());
  if (task_cache_->size() <= 0) {
    return true;
  }

  struct AssignTiebaTask task;
  MAKE_HEAD(task, ASSIGN_TIEBA_TASK, 0, 0, 0, 0, 0, srv_session->server_type(),
      srv_session->id(), 0);
  int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_TIEBA_TASK);
  LOG_DEBUG2("crawler type = %d, get from mysql", crawler_type);
  if (!crawler_schduler_engine_->CheckOptimalRouter(crawler_type)) {
    LOG_MSG2("no have OptimalCrawler with type: %d", crawler_type);
    return true;
  }

  task_cache_->PrintTaskDetail();

  cookie_cache_->SortCookies();
  base_logic::TiebaTask info;
  while (task_cache_->GetTask(info)) {
    base_logic::LoginCookie cookie;

    if (TASK_SEND_FAILED == info.state()) {
      task_db_->UpdateRobotTaskState(info.id(), TASK_SEND);
    } else {
      info.set_state(TASK_SEND);
      task_db_->RecordRobotTaskState(info);
    }
    if (FilterTask(info)) {
//      task_cache_->task_idle_list_.erase(it++);
      info.set_state(TASK_INVALID);
      continue;
    }

    struct TiebaTaskUnit *unit = new TiebaTaskUnit();
    if (0 == info.cookie().size()
        && cookie_cache_->GetCookie(info.type(), cookie)) {
      unit->cookie_id = cookie.cookie_id();
      unit->cookie = cookie.get_cookie_body();
    } else {
      unit->cookie_id = 0;
      unit->cookie = info.cookie();
    }
    LOG_MSG2("DistributionTask task_type = %lld, task_id=%d", info.type(), info.id());
    //struct TaskUnit* unit = new struct TaskUnit;
    unit->task_id = info.id();
    unit->method = info.method();
//    unit->task_type = info.type();
    unit->task_type = info.type();
    unit->addr = info.addr();
    unit->ua = info.ua();
    unit->url = info.url();
    unit->row_key = info.row_key();
    unit->task_len = TIEBA_TASK_UNIT_SIZE + unit->cookie.size()
        + unit->addr.size() + unit->ua.size() + unit->url.size()
        + unit->row_key.size();
    task.task_set.push_back(unit);
    info.set_send_time((int32) current_time);

    task_cache_->task_exec_map_[info.id()] = info;

    if (task.task_set.size() % base_num == 0 && task.task_set.size() != 0) {
      task.tasks_num = base_num;
      task.packet_length = task.data_length = data_length;
      data_length = 0;
      bool send_success = crawler_schduler_engine_->SendOptimalRouter(
          (const void*) &task, 0, crawler_type);
      net::PacketProsess::ClearTiebaTaskList(&task);
      // 一次只发一个包
      break;
    }
  }

  //解决余数
  if (task.task_set.size() > 0) {
    task.tasks_num = task.task_set.size();
    task.packet_length = task.data_length = data_length;
    data_length = 0;
    bool send_success = crawler_schduler_engine_->SendOptimalRouter((const void*) &task, 0,
        crawler_type);
    net::PacketProsess::ClearTiebaTaskList(&task);
  }

  LOG_DEBUG2("after distribution task, task_cache size = %d",
      task_cache_->size());
  return true;
}

bool TaskSchdulerManager::FilterTask(base_logic::TiebaTask &task) {
	int ret = false;
	if (task.is_main_task()) {
		return false;
	}
	base_logic::TiebaTask tieba_task;
	tieba_task.set_url(task.url());
	task_db_->FetchLastTask(tieba_task);

	time_t current_time = time(NULL);
	if (0 != tieba_task.id()
			&& tieba_task.send_time() + 30 * 60 > current_time) {
		LOG_DEBUG2("task[%lld] has been filtered, last task id: %lld, "
				"last assign time: %lld, current_time: %lld",
				task.id(), tieba_task.id(), tieba_task.send_time(), current_time);
		ret = true;
	}
	return ret;
}

bool TaskSchdulerManager::DistributionTempTask() {
//	if (NULL == session_mgr_) {
//		LOG_MSG("wait server start up");
//		return false;
//	}
//  base_logic::WLockGd lk(lock_);
//	plugin_share::ServerSession *srv_session = session_mgr_->GetServer();
//	int32 base_num = 1;
//	int data_length = 0;
//	time_t current_time = time(NULL);
//	LOG_DEBUG2("distrubute tieba task current_time=%d task_cache_->task_idle_map_.size=%d",
//			(int)current_time, task_cache_->task_idle_list_.size());
//	if (task_cache_->task_idle_list_.size() <= 0) {
//		return true;
//	}
//
//	struct AssignTiebaTask task;
//	MAKE_HEAD(task, ASSIGN_TIEBA_TASK, 0, 0, 0, 0, 0, srv_session->server_type(),
//			srv_session->id(), 0);
//	int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_TIEBA_TASK);
//	LOG_DEBUG2("crawler type = %d, get from mysql", crawler_type);
//	if (!crawler_schduler_engine_->CheckOptimalRouter(crawler_type)) {
//		LOG_MSG2("no have OptimalCrawler with type: %d", crawler_type);
//		return true;
//	}
//
//	task_cache_->PrintTaskDetail();
//
//	cookie_cache_->SortCookies();
//	int32 count = task_cache_->task_idle_list_.size();
//	int32 index = 0;
//	TASKINFO_LIST::iterator it = task_cache_->task_idle_list_.begin();
//	TASKINFO_LIST::iterator packet_start = it;
//	for (; it != task_cache_->task_idle_list_.end(), index < count; index++) {
//		base_logic::LoginCookie cookie;
//		base_logic::TiebaTask info;
//		if (info.type_list_.front() != info.type()) {
//		  continue;
//		}
//		if (info.create_time()+30*60 < current_time) {
//			LOG_DEBUG2("task timeout, url = %s, current_time = %d, create_time = %d",
//					info.url().c_str(), current_time, info.create_time());
//			info.set_state(TASK_INVALID);
//			continue;
//		}
//		if (TASK_SEND_FAILED == info.state()) {
//			task_db_->UpdateRobotTaskState(info.id(), TASK_SEND);
//		} else {
//			info.set_state(TASK_SEND);
//			task_db_->RecordRobotTaskState(info);
//		}
//		if (FilterTask(info)) {
////			task_cache_->task_idle_list_.erase(it++);
//			info.set_state(TASK_INVALID);
//			continue;
//		}
//
//		struct TiebaTaskUnit *unit = new TiebaTaskUnit();
//		if (0 == info.cookie().size()
//				&& cookie_cache_->GetCookie(info.type(), cookie)) {
//			unit->cookie_id = cookie.cookie_id();
//			unit->cookie = cookie.get_cookie_body();
//		} else {
//			unit->cookie_id = 0;
//			unit->cookie = info.cookie();
//		}
//		LOG_MSG2("DistributionTask task_type = %lld, task_id=%d", info.type(), info.id());
//		//struct TaskUnit* unit = new struct TaskUnit;
//		unit->task_id = info.id();
////		unit->task_type = info.type();
//		unit->task_type = info.type();
//		unit->addr = info.addr();
//		unit->ua = info.ua();
//		unit->url = info.url();
//		unit->row_key = info.row_key();
//		unit->task_len = TIEBA_TASK_UNIT_SIZE + unit->cookie.size()
//				+ unit->addr.size() + unit->ua.size() + unit->url.size()
//				+ unit->row_key.size();
//		task.task_set.push_back(unit);
//		info.set_send_time((int32) current_time);
////		info.update_time(current_time, base::SysRadom::GetInstance()->GetRandomID());
////		task_cache_->task_exec_map_[info.id()] = info;
////		task_cache_->task_idle_list_.erase(it++);
//		if (task.task_set.size() % base_num == 0 && task.task_set.size() != 0) {
//			task.tasks_num = base_num;
//			task.packet_length = task.data_length = data_length;
//			data_length = 0;
//			bool send_success = crawler_schduler_engine_->SendOptimalRouter(
//					(const void*) &task, 0, crawler_type);
//			if (!send_success) {
//				for (; packet_start != it; ++packet_start) {
//					packet_start->set_state(TASK_SEND_FAILED);
//					task_db_->UpdateRobotTaskState(packet_start->id(), TASK_SEND_FAILED);
//				}
//			}
//			packet_start = it;
//			net::PacketProsess::ClearTiebaTaskList(&task);
//			// 一次只发一个包
//			break;
//		}
//	}
//	base_logic::TiebaTask::TypeList &type_queue = base_logic::TiebaTask::type_list_;
//	int64 last_type = type_queue.front();
//	type_queue.pop();
//	type_queue.push(last_type);
//
//	//解决余数
//	if (task.task_set.size() > 0) {
//		task.tasks_num = task.task_set.size();
//		task.packet_length = task.data_length = data_length;
//		data_length = 0;
//		bool send_success = crawler_schduler_engine_->SendOptimalRouter((const void*) &task, 0,
//				crawler_type);
//		if (!send_success) {
//			for (; packet_start != it; ++packet_start) {
//				packet_start->set_state(TASK_SEND_FAILED);
//			}
//		}
//		net::PacketProsess::ClearTiebaTaskList(&task);
//	}
//	for (it = task_cache_->task_idle_list_.begin();
//			it != task_cache_->task_idle_list_.end();) {
//		base_logic::TiebaTask &task = *it;
//		if (TASK_INVALID == task.state()) {
//			task_cache_->task_idle_list_.erase(it++);
//		} else if (TASK_SEND == task.state()) {
//			task_cache_->task_exec_map_[task.id()] = task;
//			task_cache_->task_idle_list_.erase(it++);
//		} else {
//			++it;
//		}
//	}
//	LOG_DEBUG2("after distribution task, task_idle_list_ size = %d",
//			task_cache_->task_idle_list_.size());
//	return true;
}
void TaskSchdulerManager::CheckIsEffective() {
	crawler_schduler_engine_->CheckIsEffective();
}

uint32 TaskSchdulerManager::GetExecTasks() {
	return task_cache_->task_exec_map_.size();
}

int64& TaskSchdulerManager::GetDatabaseUpdateTimeByPlatId(const int64 plat_id) {
	return cookie_cache_->update_time_map_[plat_id];
}
}  // namespace crawler_task_logic
