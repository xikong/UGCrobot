//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry

#include <string>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"

namespace qzone_task_logic {

using namespace base_logic;

TaskSchdulerManager* TaskSchdulerEngine::schduler_mgr_ = NULL;
TaskSchdulerEngine* TaskSchdulerEngine::schduler_engine_ = NULL;

TaskSchdulerManager::TaskSchdulerManager()
:crawler_count_(0)
, task_db_(NULL)
, manager_info_(NULL) {
    task_cache_ = new TaskSchdulerCache();
	cookie_cache_ = new CookieCache();
	ip_cache_ = new IPCache();
	content_cache_ = new TaskContentCache();
	cookie_ip_manager_ = new CookieIpEngine();
    Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
    DeinitThreadrw(lock_);
}

void TaskSchdulerManager::Init() {
    InitThreadrw(&lock_);
}

void TaskSchdulerManager::InitDB(qzone_task_logic::CrawlerTaskDB* task_db) {
    task_db_ = task_db;
    SetBatchIP();
    SetBatchCookies();
    SetBatchContents();
}

void TaskSchdulerManager::Init(router_schduler::SchdulerEngine* crawler_engine) {
    crawler_schduler_engine_ = crawler_engine;
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
	base_logic::WLockGd lk(lock_);
    std::list<base_logic::LoginCookie> list;
    task_db_->GetCookies(GET_COOKIES_PER_TIME, cookie_cache_->last_time, &list);
    cookie_ip_manager_->Update(list, ip_cache_->ip_list_);
    while (list.size() > 0) {
       base_logic::LoginCookie info = list.front();
       list.pop_front();
       SetCookie(info);
    }
}

void TaskSchdulerManager::SetCookie(
        const base_logic::LoginCookie& info) {
    CookiePlatform& platform = cookie_cache_->cookie_map_[info.get_cookie_attr_id()];
    CookieList& list = platform.list;
    for (CookieList::iterator it = list.begin(); it != list.end();) {
        if (info.get_username() == it->get_username() &&
            info.get_passwd() == it->get_passwd()) {
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
    int64& plat_update_time = GetDatabaseUpdateTimeByPlatId(info.get_cookie_attr_id());
    if (plat_update_time < info_update_time)
        plat_update_time = info_update_time;

	if (info_update_time > cookie_cache_->last_time)
		cookie_cache_->last_time = plat_update_time;
}

void TaskSchdulerManager::SetBatchContents() {
	std::list<base_logic::RobotTaskContent> list;
	base_logic::WLockGd lk(lock_);
	task_db_->FetchBatchTaskContent((int16)base_logic::RobotTask::QZONE, &list);
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
	task_content.cur_it = task_content.content_list.begin();
}

void TaskSchdulerManager::FetchBatchTask(
        std::list<base_logic::QZoneTask> *list,
        bool is_first) {
    base_logic::WLockGd lk(lock_);

//    cookie_cache_->SortCookies();
    int affected_rows = 0;
    ip_cache_->SortIPBySendTime();

    time_t current_time = time(NULL);
    //======================== Test ========================
    base_logic::QZoneTask info;
    info.set_topic_id("316213308_3c08d912c0b2d2561f230d00__1");
    info.set_host_uin(316213308);
    info.set_content("不错");
    list->push_back(info);
    //======================== Test ========================
    while ((*list).size() > 0) {
        base_logic::QZoneTask info = (*list).front();
        (*list).pop_front();
        task_db_->ExistRobotTask(info, affected_rows);
        if (affected_rows > 0) {
        	LOG_MSG2("the task has exec success, type: %d, url: %s",
        			info.type(), info.url().c_str());
        	continue;
        }
        task_cache_->task_idle_map_[info.id()] = info;
    }
}

bool TaskSchdulerManager::AlterTaskState(const int64 task_id,
        const int8 state) {
    base_logic::WLockGd lk(lock_);
    task_cache_->task_exec_map_[task_id].set_state((TASKSTAE)state);
    if (TASK_EXECUED == state) {
        task_cache_->task_exec_map_.erase(task_id);
        LOG_DEBUG2("after erase, task_exec_map size = %d",
        		task_cache_->task_exec_map_.size());
    }
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
    TASKINFO_MAP::iterator it =
            task_cache_->task_exec_map_.begin();
    time_t current_time = time(NULL);
    for (; it != task_cache_->task_exec_map_.end();) {
        base_logic::QZoneTask& task = it->second;
        if ((task.send_time() + 60)
              <  current_time) {
            task_db_->UpdateRobotTaskState(task.id(), TASK_WAIT);
            task_cache_->task_exec_map_.erase(it++);
            LOG_DEBUG2("after erase, task_exec_map size = %d",
            		task_cache_->task_exec_map_.size());
        } else {
            it++;
        }
    }
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

bool TaskSchdulerManager::DistributionTask() {
	if (NULL == manager_info_) {
		LOG_MSG("wait server init complete");
		return false;
	}
    int32 base_num = 5;
    int data_length = 0;
    time_t current_time = time(NULL);
	LOG_DEBUG2("distrubute qzone task current_time=%d task_cache_->task_idle_map_.size=%d", (int)current_time, task_cache_->task_idle_map_.size());
    if (task_cache_->task_idle_map_.size() <= 0) {
    	return true;
    }
    if (!crawler_schduler_engine_->CheckOptimalRouter()) {
        LOG_MSG("no have OptimalCrawler");
        return true;
    }

    cookie_cache_->SortCookies();

    struct AssignQzoneTask task;
    MAKE_HEAD(task, ASSIGN_QZONE_TASK, 0, 0, 0, 0, 0, 0, manager_info_->svr_info.id, 0);
    task.task_type = (uint16)base_logic::RobotTask::QZONE;

    int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_QZONE_TASK);
    //LOG_DEBUG2("crawler type = %d, get from mysql", crawler_type);
    base_logic::WLockGd lk(lock_);
    int32 count = task_cache_->task_idle_map_.size();
    int32 index = 0;
    TASKINFO_MAP::iterator it = task_cache_->task_idle_map_.begin();
    for (; it != task_cache_->task_idle_map_.end(), index < count; index++) {
    	base_logic::LoginCookie cookie;
        base_logic::QZoneTask& info = it->second;
        if (cookie_cache_->GetCookie(info.type(), cookie)) {
        	std::string addr;
        	if (!cookie_ip_manager_->GetIpByCookie(cookie, addr)) {
        		LOG_MSG2("there are no ip for cookie: %s",
        				cookie.get_cookie_body().c_str());
        				continue;
        	}
        	task_db_->RecordRobotTaskState(info);
			LOG_MSG2("DistributionTask task_id=%d", info.id());
            //struct TaskUnit* unit = new struct TaskUnit;
			struct QzoneTaskUnit *unit = new QzoneTaskUnit();
			unit->task_id = info.id();

			unit->cookie_id = cookie.cookie_id();
			unit->cookie = cookie.get_cookie_body();

			std::string con;
			if (!content_cache_->GetContentByTaskType(info.type(), con)) {
				LOG_MSG2("task(id: %d, type: %d) has no content, ignore it",
						info.id(), info.type());
				delete unit;
				continue;
			}
			unit->content = con;

			unit->addr = addr;
			unit->topic_id = info.topic_id();
			unit->host_uin = info.host_uin();
            task.task_set.push_back(unit);
            info.set_state(TASK_SEND);
            info.set_send_time();
            task_cache_->task_exec_map_[info.id()] = info;
            LOG_DEBUG2("after insert, task_exec_map size = %d",
            		task_cache_->task_exec_map_.size());
            task_cache_->task_idle_map_.erase(it++);
            if (task.task_set.size() % base_num == 0 &&
					task.task_set.size() != 0) {
            	task.task_type = info.type();
            	task.tasks_num = base_num;
				task.packet_length = task.data_length = data_length;
				data_length = 0;
				crawler_schduler_engine_->SendOptimalRouter((const void*)&task, 0, crawler_type);
				net::PacketProsess::ClearQzoneTaskList(&task);
			}
        } else {
        	++it;
        }
    }

    //解决余数
	if (task.task_set.size() > 0) {
		task.tasks_num = task.task_set.size();
		task.packet_length = task.data_length = data_length;
		data_length = 0;
		crawler_schduler_engine_->SendOptimalRouter((const void*)&task, 0, crawler_type);
		net::PacketProsess::ClearQzoneTaskList(&task);
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
