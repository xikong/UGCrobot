//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry

#include <string>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"
#include "net/packet_processing.h"
#include "crawler_task_logic.h"

namespace robot_task_logic {

using namespace base_logic;

TaskSchdulerManager* TaskSchdulerEngine::schduler_mgr_ = NULL;
TaskSchdulerEngine* TaskSchdulerEngine::schduler_engine_ = NULL;

bool CookieCache::GetCookie(int64 attr_id, base_logic::LoginCookie &cookie) {
	time_t current_time = time(NULL);
	if (cookie_map_.end() == cookie_map_.find(attr_id)) {
		LOG_MSG2("can't find the cookie with the attr_id: %d", attr_id);
		return false;
	}
	struct CookiePlatform &platform = cookie_map_[attr_id];
	if (platform.list.end() == platform.cur_it)
		platform.cur_it = platform.list.begin();
	cookie = *platform.cur_it;
	if ((cookie.send_last_time() + config_->cookie_use_tick) > current_time) {
		LOG_MSG("cookie use too often");
		return false;
	}
	++platform.cur_it;
	cookie.update_time();
	return true;
}

TaskSchdulerManager::TaskSchdulerManager() :
		crawler_count_(0), task_db_(NULL), manager_info_(NULL) {
	task_cache_ = new TaskSchdulerCache();
	cookie_cache_ = new CookieCache();
	ip_cache_ = new IPCache();
	content_cache_ = new TaskContentCache();
	cookie_ip_manager_ = new CookieIpEngine();
	ua_cache_ = new ForgeryUACache();
	Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
	DeinitThreadrw(lock_);
}

void TaskSchdulerManager::Init() {
	InitThreadrw(&lock_);
}

void TaskSchdulerManager::InitDB(robot_task_logic::CrawlerTaskDB* task_db) {
	task_db_ = task_db;
	cookie_cache_->SetTaskDb(task_db);
	SetBatchIP();
	task_db_->FectchBatchForgeryUA(&ua_cache_->ua_list_);
	SetBatchCookies();
	SetBatchContents();

	// test
	cookie_cache_->BindForgeryIP(*ip_cache_);
	cookie_cache_->BindForgeryUA(*ua_cache_);
	cookie_cache_->SortCookies();
}

void TaskSchdulerManager::Init(
		router_schduler::SchdulerEngine* crawler_engine, Config *config) {
	crawler_schduler_engine_ = crawler_engine;
	config_ = config;
	cookie_cache_->SetConfig(config);
}

void TaskSchdulerManager::InitManagerInfo(plugin_share::ManagerInfo *info) {
	manager_info_ = info;
}

void TaskSchdulerManager::SetBatchIP() {
	base_logic::WLockGd lk(lock_);
	std::list<base_logic::ForgeryIP> ip_list;
	task_db_->FectchBatchForgeryIP(&ip_list);
	ip_cache_->Update(ip_list);
//	ip_cache_->Reset();
	cookie_ip_manager_->Update(ip_list);
}

void TaskSchdulerManager::SetBatchCookies() {
	std::list<base_logic::LoginCookie> list;
	base_logic::WLockGd lk(lock_);
	task_db_->GetCookies(GET_COOKIES_PER_TIME, cookie_cache_->last_time, &list);
	cookie_ip_manager_->Update(list, ip_cache_->ip_list_);
	while (list.size() > 0) {
		base_logic::LoginCookie info = list.front();
		list.pop_front();
		SetCookie(info);
	}
}

bool TaskSchdulerManager::RemoveInvalidCookie(int64 cookie_id) {
	return cookie_cache_->RemoveInvalidCookie(cookie_id);
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

void TaskSchdulerManager::SetBatchContents() {
	std::list<base_logic::RobotTaskContent> list;
	base_logic::WLockGd lk(lock_);
	task_db_->FetchBatchTaskContent((int16) base_logic::RobotTask::TIEBA,
			&list);
	LOG_DEBUG2("task content size: %d", list.size());
	while (list.size() > 0) {
		base_logic::RobotTaskContent &con = list.front();
		SetContent(con);
		list.pop_front();
	}
}

void TaskSchdulerManager::SetContent(const base_logic::RobotTaskContent &con) {
	TaskContent &task_content = content_cache_->content_map_[con.task_type()];
	task_content.task_type = con.task_type();
	task_content.content_list.push_back(con);
	LOG_DEBUG2("task type: %d, task content size: %d",
			con.task_type(), task_content.content_list.size());
	task_content.cur_it = task_content.content_list.begin();
}

void TaskSchdulerManager::FetchBatchTask(
		std::list<base_logic::RobotTask *> *list, bool is_first) {
	base_logic::WLockGd lk(lock_);
	base_logic::ForgeryUA ua;
	time_t current_time = time(NULL);
//	ua_cache_->SortUABySendTime();
	while ((*list).size() > 0) {
		base_logic::RobotTask *info = (*list).front();
		(*list).pop_front();
		ua_cache_->GetUA(ua);
//		info->set_ua(ua);
		task_cache_->task_idle_map_[info->id()] = info;
	}
}

bool TaskSchdulerManager::AlterTaskState(const int64 task_id,
		const int8 state) {
	base_logic::WLockGd lk(lock_);
	if (task_cache_->task_exec_map_.end()
			== task_cache_->task_exec_map_.find(task_id)) {
		LOG_MSG2("don't find the task, task_id = %lld", task_id);
		return false;
	}
	task_cache_->task_exec_map_[task_id]->set_state((TaskState) state);
	if (TASK_EXECUED == state) {
		task_cache_->task_exec_map_.erase(task_id);
		LOG_DEBUG2("after erase, task_exec_map size = %d",
				task_cache_->task_exec_map_.size());
	}
	return true;
}

void TaskSchdulerManager::RecyclingTask() {
	base_logic::WLockGd lk(lock_);
	TASKINFO_MAP::iterator it = task_cache_->task_exec_map_.begin();
	time_t current_time = time(NULL);
	for (; it != task_cache_->task_exec_map_.end();) {
		base_logic::RobotTask *task = it->second;
		if ((task->send_time() + 60) < current_time) {
			task_db_->UpdateRobotTaskState(task->id(), (int)TASK_WAIT, std::string("none"));
			delete it->second;
			task_cache_->task_exec_map_.erase(it++);
			LOG_DEBUG2("after erase, task_exec_map size = %d",
					task_cache_->task_exec_map_.size());
		} else {
			it++;
		}
	}
}

bool TaskSchdulerManager::DistributionTask() {
	if (NULL == manager_info_) {
		LOG_MSG("wait server init complete");
		return false;
	}
	int32 base_num = 1;
	int data_length = 0;
	time_t current_time = time(NULL);
	LOG_DEBUG2("distrubute task current_time=%d task_cache_->task_idle_map_.size=%d",
			(int)current_time, task_cache_->task_idle_map_.size());
	if (task_cache_->task_idle_map_.size() <= 0) {
		return true;
	}

	struct RobotTasks tasks;
	MAKE_HEAD(tasks, ASSIGN_ROBOT_TASKS, 0, 0, 0, 0, 0, manager_info_->svr_info.type,
			manager_info_->svr_info.id, 0);
	int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_ROBOT_TASKS);
	LOG_DEBUG2("task_type[%d] map to crawler_type[%d]", ASSIGN_ROBOT_TASKS, crawler_type);
	if (!crawler_schduler_engine_->CheckOptimalRouter(crawler_type)) {
		LOG_MSG2("no have OptimalCrawler with crawler_type: %d", crawler_type);
		return true;
	}

	base_logic::WLockGd lk(lock_);
	int32 count = task_cache_->task_idle_map_.size();
	int32 index = 0;

	cookie_cache_->BindForgeryIP(*ip_cache_);
	cookie_cache_->BindForgeryUA(*ua_cache_);
	cookie_cache_->SortCookies();

	TASKINFO_MAP::iterator it = task_cache_->task_idle_map_.begin();
	TASKINFO_MAP::iterator packet_start = it;
	for (; it != task_cache_->task_idle_map_.end(), index < count; index++) {
		base_logic::RobotTaskContent	con;
		base_logic::ForgeryIP 			ip;
		base_logic::LoginCookie 		cookie;
		base_logic::RobotTask *info = (it++)->second;
		if (!cookie_cache_->GetCookie(info->type(), cookie))
			continue;
//		if (!cookie_ip_manager_->GetIpByCookie(cookie, ip)) {
//			LOG_MSG2("there are no ip for cookie: %s",
//					cookie.get_cookie_body().c_str());
//			continue;
//		}
		if (!content_cache_->GetContentByTaskType(info->type(), con)) {
			LOG_MSG2("task(id: %d, type: %d) has no content, ignore it",
					info->id(), info->type());
			continue;
		}

		info->set_cookie(cookie);
//		info->set_ip(ip);
		info->set_content(con);

		struct RobotTaskBase *unit = info->CreateTaskPacketUnit();

		tasks.task_set.push_back(unit);
		info->set_state(TASK_SEND);
		info->set_send_time();
//		task_cache_->task_exec_map_[info.id()] = info;
//		LOG_DEBUG2("after insert, task_exec_map size = %d",
//				task_cache_->task_exec_map_.size());
//		task_cache_->task_idle_map_.erase(it++);
		task_db_->UpdateRobotTaskDetail(info);
		LOG_MSG2("DistributionTask task_type = %d, task_id=%d", info->type(), info->id());
		if (tasks.task_set.size() % base_num == 0
				&& tasks.task_set.size() != 0) {
			tasks.task_num = tasks.task_set.size();
			bool send_success = true;
//			net::PacketProsess::DumpPacket(&tasks);
			send_success = crawler_schduler_engine_->SendOptimalRouter(
					(const void*) &tasks, 0, crawler_type);
			if (!send_success) {
				LOG_DEBUG2("packet_start ~ it size: %d",
						std::distance(packet_start, it));
				for (; packet_start != it; ++packet_start) {
					base_logic::RobotTask *task = packet_start->second;
					task->set_state(TASK_SEND_FAILED);
					task_db_->UpdateRobotTaskState(task->id(), (int)task->state(), std::string("none"));
				}
			}
			packet_start = it;
			net::PacketProsess::ClearRobotTaskList(&tasks);
			// 每次只分发一个包
			break;
		}
	 }

	//解决余数
	if (tasks.task_set.size() > 0) {
		tasks.task_num = tasks.task_set.size();
		bool send_success = true;
//		net::PacketProsess::DumpPacket(&tasks);
		send_success = crawler_schduler_engine_->SendOptimalRouter((const void*) &tasks, 0,
				crawler_type);
		if (!send_success) {
			LOG_DEBUG2("packet_start ~ packet_start size: %d",
					std::distance(packet_start, it));
			for (; packet_start != it; ++packet_start) {
				base_logic::RobotTask *task = packet_start->second;
				task->set_state(TASK_SEND_FAILED);
				task_db_->UpdateRobotTaskState(task->id(), (int)task->state(), std::string("none"));
			}
		}
		net::PacketProsess::ClearRobotTaskList(&tasks);
	}

	//
	for (it = task_cache_->task_idle_map_.begin();
			it != task_cache_->task_idle_map_.end();) {
		base_logic::RobotTask *task = it->second;
		if (TASK_INVALID == task->state()) {
			task_cache_->task_idle_map_.erase(it++);
		} else if (TASK_SEND == task->state()) {
			task_cache_->task_exec_map_[task->id()] = task;
			task_cache_->task_idle_map_.erase(it++);
		} else {
			++it;
		}
	}
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
