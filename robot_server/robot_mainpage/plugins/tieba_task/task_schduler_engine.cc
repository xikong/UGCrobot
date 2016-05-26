//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry

#include <string>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"

namespace tieba_task_logic {

using namespace base_logic;

TaskSchdulerManager* TaskSchdulerEngine::schduler_mgr_ = NULL;
TaskSchdulerEngine* TaskSchdulerEngine::schduler_engine_ = NULL;

TaskSchdulerManager::TaskSchdulerManager() :
		crawler_count_(0), task_db_(NULL), session_mgr_(NULL) {
	task_cache_ = new TaskSchdulerCache();
	cookie_cache_ = new CookieCache();
	ip_cache_ = new IPCache();
	ua_cache_ = new ForgeryUACache();
	Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
	DeinitThreadrw(lock_);
}

void TaskSchdulerManager::Init() {
	InitThreadrw(&lock_);
}

void TaskSchdulerManager::InitDB(tieba_task_logic::CrawlerTaskDB* task_db) {
	task_db_ = task_db;
	task_db_->FectchBatchForgeryIP(&ip_cache_->ip_list_);
	task_db_->FectchBatchForgeryUA(&ua_cache_->ua_list_);
	LOG_MSG2("forgery ip size: %d, forgery ua size: %d",
			ip_cache_->ip_list_.size(), ua_cache_->ua_list_.size());
	SetBatchCookies();
}

void TaskSchdulerManager::FetchIP() {
  base_logic::WLockGd lk(lock_);
  ip_cache_->ip_list_.clear();
  task_db_->FectchBatchForgeryIP(&ip_cache_->ip_list_);
  LOG_DEBUG2("forgery ip size: %d", ip_cache_->ip_list_.size());
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
			LOG_MSG2("erase old cookie username=%s passwd=%s",
					info.get_username().c_str(), info.get_passwd());
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
	ip_cache_->SortIPBySendTime();
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
		base_logic::ForgeryIP ip;
		base_logic::ForgeryUA ua;
		ip_cache_->GetIP(ip);
		ua_cache_->GetUA(ua);
		info.set_addr(ip.ip());
		info.set_ua(ua.ua());

		base::SysRadom::GetInstance()->InitRandom();
		info.update_time(current_time,
				base::SysRadom::GetInstance()->GetRandomID(), is_first);
		info.set_id(base::SysRadom::GetInstance()->GetRandomID());
//        task_cache_->task_idle_list_[info.id()] = info;
		LOG_DEBUG2("fetch task, task_type = %lld, task_idle_list.size() = %d",
				info.type(), task_cache_->task_idle_list_.size());
//		if (info.is_main_task()) {
//			task_cache_->task_idle_list_.remove_if(IsMainTask);
//			LOG_DEBUG2("after remove main tasks, task_idle_list.size() = %d",
//					task_cache_->task_idle_list_.size());
//		}
		task_cache_->task_idle_list_.push_back(info);
		base::SysRadom::GetInstance()->DeinitRandom();
	}
}

void TaskSchdulerManager::FetchBatchTemp(
		std::list<base_logic::TiebaTask>* list) {
#if 0
	base_logic::WLockGd lk(lock_);
	while ((*list).size() > 0) {
		base_logic::TiebaTask info = (*list).front();
		(*list).pop_front();
		task_cache_->task_temp_list_.push_back(info);
	}
#endif
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

bool TaskSchdulerManager::DistributionTempTask() {
	/*
	 LOG_MSG2("task_temp_list_ size %d", task_cache_->task_temp_list_.size());
	 if (task_cache_->task_temp_list_.size() <= 0)
	 return true;
	 if (!crawler_schduler_engine_->CheckOptimalRouter()) {
	 LOG_MSG("no have OptimalCrawler");
	 return true;
	 }
	 int32 base_num = 5;
	 struct AssignmentMultiTask  task;
	 int data_length = 0;
	 MAKE_HEAD(task, ASSIGNMENT_MULTI_TASK, 0, 0, 0,0, 0,0 , 0, 0);
	 int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGNMENT_MULTI_TASK);
	 LOG_DEBUG2("DistributionTempTask: crawler type = %d", crawler_type);
	 base_logic::WLockGd lk(lock_);

	 while (task_cache_->task_temp_list_.size() > 0) {
	 base_logic::TaskInfo info = task_cache_->task_temp_list_.front();
	 task_cache_->task_temp_list_.pop_front();
	 task_db_->RecordTaskState(info, 1);
	 if ((info.state() == TASK_WAIT || info.state() == TASK_EXECUED)) {
	 struct TaskUnit* unit = new struct TaskUnit;
	 unit->task_id = info.id();
	 unit->attr_id = info.attrid();
	 unit->max_depth = info.depth();
	 unit->current_depth = info.cur_depth();
	 unit->machine = info.machine();
	 unit->storage = info.storage();
	 unit->is_login = info.is_login();
	 unit->is_over = info.is_over();
	 unit->is_forge = info.is_forge();
	 unit->method = info.method();

	 memset(unit->url, '\0', URL_SIZE);
	 size_t url_len = (URL_SIZE - 1) < info.url().length() ?  (URL_SIZE - 1) : info.url().length();
	 memcpy(unit->url, info.url().c_str(), url_len);

	 unit->len = TASK_UNIT_SIZE + url_len;
	 data_length += unit->len;

	 task.task_set.push_back(unit);
	 info.set_state(TASK_SEND);
	 task_cache_->task_exec_map_[info.id()] = info;
	 if (task.task_set.size() % base_num == 0 &&
	 task.task_set.size() != 0) {
	 task.packet_length = task.data_length = data_length;
	 data_length = 0;
	 crawler_schduler_engine_->SendOptimalRouter((const void*)&task, 0, crawler_type);
	 net::PacketProsess::ClearCrawlerTaskList(&task);
	 }
	 }
	 }

	 //解决余数
	 if (task.task_set.size() > 0) {
	 task.packet_length = task.data_length = data_length;
	 data_length = 0;
	 crawler_schduler_engine_->SendOptimalRouter((const void*)&task, 0, crawler_type);
	 net::PacketProsess::ClearCrawlerTaskList(&task);
	 }

	 LOG_MSG2("task_temp_list_ size %d", task_cache_->task_temp_list_.size());
	 return true;
	 */
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

bool TaskSchdulerManager::DistributionTask() {
	if (NULL == session_mgr_) {
		LOG_MSG("wait server start up");
		return false;
	}
	base_logic::MLockGd lock(session_mgr_->lock_);
	plugin_share::ServerSession *srv_session = session_mgr_->GetServer();
	int32 base_num = 1;
	int data_length = 0;
	time_t current_time = time(NULL);
	LOG_DEBUG2("distrubute tieba task current_time=%d task_cache_->task_idle_map_.size=%d",
			(int)current_time, task_cache_->task_idle_list_.size());
	if (task_cache_->task_idle_list_.size() <= 0) {
		return true;
	}

	cookie_cache_->SortCookies();

	struct AssignTiebaTask task;
	MAKE_HEAD(task, ASSIGN_TIEBA_TASK, 0, 0, 0, 0, 0, srv_session->server_type(),
			srv_session->id(), 0);
	int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_TIEBA_TASK);
	LOG_DEBUG2("crawler type = %d, get from mysql", crawler_type);
	if (!crawler_schduler_engine_->CheckOptimalRouter(crawler_type)) {
		LOG_MSG2("no have OptimalCrawler with type: %d", crawler_type);
		return true;
	}

	base_logic::WLockGd lk(lock_);
	int32 count = task_cache_->task_idle_list_.size();
	int32 index = 0;
	TASKINFO_LIST::iterator it = task_cache_->task_idle_list_.begin();
	TASKINFO_LIST::iterator packet_start = it;
	for (; it != task_cache_->task_idle_list_.end(), index < count; index++) {
		base_logic::LoginCookie cookie;
		base_logic::TiebaTask& info = *it++;
		if (info.type_queue_.front() != info.type()) {
		  continue;
		}
		if (info.create_time()+30*60 < current_time) {
			LOG_DEBUG2("task timeout, url = %s, current_time = %d, create_time = %d",
					info.url().c_str(), current_time, info.create_time());
			info.set_state(TASK_INVALID);
			continue;
		}
		if (TASK_SEND_FAILED == info.state()) {
			task_db_->UpdateRobotTaskState(info.id(), TASK_SEND);
		} else {
			info.set_state(TASK_SEND);
			task_db_->RecordRobotTaskState(info);
		}
		if (FilterTask(info)) {
//			task_cache_->task_idle_list_.erase(it++);
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
//		unit->task_type = info.type();
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
//		info.update_time(current_time, base::SysRadom::GetInstance()->GetRandomID());
//		task_cache_->task_exec_map_[info.id()] = info;
//		task_cache_->task_idle_list_.erase(it++);
		if (task.task_set.size() % base_num == 0 && task.task_set.size() != 0) {
			task.tasks_num = base_num;
			task.packet_length = task.data_length = data_length;
			data_length = 0;
			bool send_success = crawler_schduler_engine_->SendOptimalRouter(
					(const void*) &task, 0, crawler_type);
			if (!send_success) {
				for (; packet_start != it; ++packet_start) {
					packet_start->set_state(TASK_SEND_FAILED);
					task_db_->UpdateRobotTaskState(packet_start->id(), TASK_SEND_FAILED);
				}
			}
			packet_start = it;
			net::PacketProsess::ClearTiebaTaskList(&task);
			// 一次只发一个包
			break;
		}
	}
	base_logic::TiebaTask::TypeQueue &type_queue = base_logic::TiebaTask::type_queue_;
	int64 last_type = type_queue.front();
	type_queue.pop();
	type_queue.push(last_type);

	//解决余数
	if (task.task_set.size() > 0) {
		task.tasks_num = task.task_set.size();
		task.packet_length = task.data_length = data_length;
		data_length = 0;
		bool send_success = crawler_schduler_engine_->SendOptimalRouter((const void*) &task, 0,
				crawler_type);
		if (!send_success) {
			for (; packet_start != it; ++packet_start) {
				packet_start->set_state(TASK_SEND_FAILED);
			}
		}
		net::PacketProsess::ClearTiebaTaskList(&task);
	}
	for (it = task_cache_->task_idle_list_.begin();
			it != task_cache_->task_idle_list_.end();) {
		base_logic::TiebaTask &task = *it;
		if (TASK_INVALID == task.state()) {
			task_cache_->task_idle_list_.erase(it++);
		} else if (TASK_SEND == task.state()) {
			task_cache_->task_exec_map_[task.id()] = task;
			task_cache_->task_idle_list_.erase(it++);
		} else {
			++it;
		}
	}
	LOG_DEBUG2("after distribution task, task_idle_list_ size = %d",
			task_cache_->task_idle_list_.size());
	return true;
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
