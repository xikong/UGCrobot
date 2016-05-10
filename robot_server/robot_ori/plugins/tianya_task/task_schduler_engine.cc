//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月23日 Author: kerry


#include <string>
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "task_schduler_engine.h"

namespace tianya_task_logic {

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
    Init();
}

TaskSchdulerManager::~TaskSchdulerManager() {
    DeinitThreadrw(lock_);
}

void TaskSchdulerManager::Init() {
    InitThreadrw(&lock_);
}

void TaskSchdulerManager::InitDB(tianya_task_logic::CrawlerTaskDB* task_db) {
    task_db_ = task_db;
    task_db_->FectchBatchForgeryIP(&ip_cache_->ip_list_);
    SetBatchCookies();
    SetBatchContents();
}

void TaskSchdulerManager::Init(router_schduler::SchdulerEngine* crawler_engine) {
    crawler_schduler_engine_ = crawler_engine;
}

void TaskSchdulerManager::InitManagerInfo(plugin_share::ManagerInfo *info) {
	manager_info_ = info;
}

void TaskSchdulerManager::SetBatchCookies() {
    std::list<base_logic::LoginCookie> list;
    base_logic::WLockGd lk(lock_);
    task_db_->GetCookies(GET_COOKIES_PER_TIME, cookie_cache_->last_time, &list);
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
	task_db_->FetchBatchTaskContent((int16)base_logic::RobotTask::TIANYA, &list);
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
        std::list<base_logic::TianyaTask> *list,
        bool is_first) {
    base_logic::WLockGd lk(lock_);

//    cookie_cache_->SortCookies();
    ip_cache_->SortIPBySendTime();

    time_t current_time = time(NULL);
    //======================== Test ========================
	base_logic::TianyaTask task;
	task.set_url("http://bbs.tianya.cn/post-water-1727544-1.shtml");
	task.set_title("papa酱都被封了");
	task.set_user_id("111377598");
	task.set_username("liziyun5378");
	task.set_post_time(1460976600000);
	task.set_content("HahaTest");
	list->push_back(task);
    //======================== Test ========================
    while ((*list).size() > 0) {
        base_logic::TianyaTask info = (*list).front();
        (*list).pop_front();
        base_logic::ForgeryIP ip;
        ip_cache_->GetIP(ip);
        info.set_addr(ip.ip());
        task_cache_->task_idle_map_[info.id()] = info;
    }
}

void TaskSchdulerManager::FetchBatchTemp(std::list<base_logic::TianyaTask>* list) {
#if 0
    base_logic::WLockGd lk(lock_);
    while ((*list).size() > 0) {
        base_logic::QZoneTask info = (*list).front();
        (*list).pop_front();
        task_cache_->task_temp_list_.push_back(info);
    }
#endif
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
        base_logic::TianyaTask &task = it->second;
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
	LOG_DEBUG2("distrubute tianya task current_time=%d task_cache_->task_idle_map_.size=%d", (int)current_time, task_cache_->task_idle_map_.size());
    if (task_cache_->task_idle_map_.size() <= 0) {
    	return true;
    }
    if (!crawler_schduler_engine_->CheckOptimalRouter()) {
        LOG_MSG("no have OptimalCrawler");
        return true;
    }

    cookie_cache_->SortCookies();

    struct AssignTianyaTask task;
    MAKE_HEAD(task, ASSIGN_TIANYA_TASK, 0, 0, 0, 0, 0, 0, manager_info_->svr_info.id, 0);
    task.task_type = (uint16)base_logic::RobotTask::TIANYA;

    int crawler_type = task_db_->GetCrawlerTypeByOpCode(ASSIGN_TIANYA_TASK);
    LOG_DEBUG2("crawler type = %d, get from mysql", crawler_type);
    base_logic::WLockGd lk(lock_);
    int32 count = task_cache_->task_idle_map_.size();
    int32 index = 0;
    TASKINFO_MAP::iterator it = task_cache_->task_idle_map_.begin();
    for (; it != task_cache_->task_idle_map_.end(), index < count; index++) {
    	base_logic::LoginCookie cookie;
        base_logic::TianyaTask& info = it->second;
        if (cookie_cache_->GetCookie(info.type(), cookie)) {
			task_db_->RecordRobotTaskState(info);
			LOG_MSG2("DistributionTask task_type = %d, task_id=%d", info.type(), info.id());
            //struct TaskUnit* unit = new struct TaskUnit;
			struct TianyaTaskUnit *unit = new TianyaTaskUnit();
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

			unit->addr = info.addr();
			unit->pre_post_time = info.post_time();
			unit->pre_url = info.url();
			unit->pre_title = info.title();
			unit->pre_user_id = info.user_id();
			unit->pre_user_name = info.username();
            task.task_set.push_back(unit);
            info.set_state(TASK_SEND);
            info.set_send_time();
//            info.update_time(current_time, base::SysRadom::GetInstance()->GetRandomID());
            task_cache_->task_exec_map_[info.id()] = info;
            LOG_DEBUG2("after insert, task_exec_map size = %d",
            		task_cache_->task_exec_map_.size());
            task_cache_->task_idle_map_.erase(it++);
            if (task.task_set.size() % base_num == 0 &&
					task.task_set.size() != 0) {
            	task.tasks_num = base_num;
				task.packet_length = task.data_length = data_length;
				data_length = 0;
				crawler_schduler_engine_->SendOptimalRouter((const void*)&task, 0, crawler_type);
				net::PacketProsess::ClearTianyaTaskList(&task);
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
		net::PacketProsess::ClearTianyaTaskList(&task);
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
