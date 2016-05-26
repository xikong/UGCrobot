//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#include <sstream>
#include "logic/auto_crawler_infos.h"
#include "basic/radom_in.h"
#include "basic/basic_util.h"

namespace base_logic {

CrawlerScheduler::CrawlerScheduler() {
    data_ = new Data();
}

CrawlerScheduler::CrawlerScheduler(const CrawlerScheduler& crawler_scheduler)
:data_(crawler_scheduler.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

CrawlerScheduler& CrawlerScheduler::operator =(
        const CrawlerScheduler& crawler_scheduler) {
    if (crawler_scheduler.data_ != NULL) {
        crawler_scheduler.data_->AddRef();
    }

    if (data_ != NULL) {
        data_->Release();
    }

    data_ = crawler_scheduler.data_;
    return (*this);
}

RouterScheduler::RouterScheduler() {
  data_ = new Data();
}


RouterScheduler::RouterScheduler(const RouterScheduler& router_scheduler)
:data_(router_scheduler.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}


RouterScheduler& RouterScheduler::operator =(const RouterScheduler& router_scheduler) {
	if (router_scheduler.data_ != NULL) {
		router_scheduler.data_->AddRef();
	}
    if (data_ != NULL) {
        data_->Release();
    }
    data_ = router_scheduler.data_;
    return (*this);
}

void RouterScheduler::set_router_status(struct RouterStatus *router_status,
		int socket) {
	set_recv_last_time(time(NULL));
	std::list<struct RouterStatusUnit *>::iterator it =
			router_status->status_list.begin();
	int16 router_id = router_status->router_id;
	data_->crawler_type_tasks_sum_map_.clear();
	for (; it != router_status->status_list.end(); ++it) {
		int16 crawler_type = (*it)->crawler_type;
		uint32 max_tasks = (*it)->max_tasks;
		uint32 cur_tasks = (*it)->cur_tasks;
		int32 idle_tasks = max_tasks - cur_tasks;
		LOG_DEBUG2("router[%d] status: crawler_type = %d, max_tasks = %d, cur_tasks = %d, idle_tasks = %d", router_status->router_id, crawler_type, max_tasks, cur_tasks, idle_tasks);
		data_->crawler_type_tasks_sum_map_[crawler_type] = idle_tasks;
	}
}

TaskInfo::TaskInfo() {
    data_ = new Data();
}

TaskInfo::TaskInfo(const TaskInfo& task)
:data_(task.data_) {
    if (data_ != NULL) {
       data_->AddRef();
    }
}

TaskInfo& TaskInfo::operator =(const TaskInfo& task) {
    if (task.data_ != NULL) {
        task.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }

    data_ = task.data_;
    return (*this);
}

void TaskInfo::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &data_->id_);
    dict->GetCharInteger(L"depth", &data_->depth_);
    dict->GetCharInteger(L"machine", &data_->machine_);
    dict->GetCharInteger(L"storage", &data_->storage_);
    dict->GetCharInteger(L"islogin", &data_->is_login_);
    dict->GetCharInteger(L"isforge", &data_->is_forge_);
    dict->GetCharInteger(L"isover", &data_->is_over_);
    dict->GetCharInteger(L"method", &data_->method_);

    int8 cur_depth = 1;
    if (dict->GetCharInteger(L"cur_depth", &cur_depth))
        data_->cur_depth_ = cur_depth;

    int64 cur_time = time(NULL);
    if (dict->GetBigInteger(L"cur_time", &cur_time))
        data_->create_time_ = cur_depth;

    dict->GetBigInteger(L"attrid", &data_->attrid_);
    dict->GetBigInteger(L"polling_time", &data_->base_polling_time_);
    dict->GetString(L"url", &data_->url_);
}

WeiboTaskInfo::WeiboTaskInfo() {
    data_ = new Data();
}

WeiboTaskInfo::WeiboTaskInfo(const WeiboTaskInfo& task)
:data_(task.data_) {
    if (data_ != NULL) {
       data_->AddRef();
    }
}

WeiboTaskInfo& WeiboTaskInfo::operator =(const WeiboTaskInfo& task) {
    if (task.data_ != NULL) {
        task.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }

    data_ = task.data_;
    return (*this);
}

void WeiboTaskInfo::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &data_->id_);
}

ForgeryIP::ForgeryIP() {
    data_ = new Data();
}

ForgeryIP::ForgeryIP(const ForgeryIP& ip)
:data_(ip.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

ForgeryIP& ForgeryIP::operator =(const ForgeryIP& ip) {
    if (ip.data_ != NULL) {
        ip.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }

    data_ = ip.data_;
    return (*this);
}
bool ForgeryIP::cmp(const ForgeryIP& t_info, const ForgeryIP& r_info) {
    return t_info.send_last_time() < r_info.send_last_time();
}

void ForgeryIP::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetInteger(L"id", &data_->id_);
    //  dict->GetInteger(L"type", reinterpret_cast<int32*>(&data_->type_));
    dict->GetCharInteger(L"type", &data_->type_);
    dict->GetString(L"ip", &data_->ip_);
    dict->GetString(L"create_time", &data_->create_time_);
}

ForgeryUA::ForgeryUA() {
    data_ = new Data();
}

ForgeryUA::ForgeryUA(const ForgeryUA& ua)
:data_(ua.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

ForgeryUA& ForgeryUA::operator =(const ForgeryUA& ua) {
    if (ua.data_ != NULL) {
        ua.data_->AddRef();
    }

    if (data_ != NULL) {
        data_->Release();
    }
    data_ = ua.data_;
    return (*this);
}

void ForgeryUA::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetInteger(L"id", &data_->id_);
    //  dict->GetInteger(L"type", reinterpret_cast<int32*>(&data_->type_));
    dict->GetCharInteger(L"type", &data_->type_);
    dict->GetString(L"ua", &data_->ua_);
    dict->GetString(L"create_time", &data_->create_time_);
}

TaskPlatDescription::TaskPlatDescription() {
    data_ = new Data();
}

TaskPlatDescription::TaskPlatDescription(const TaskPlatDescription& task_info)
:data_(task_info.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

TaskPlatDescription& TaskPlatDescription::operator =(
        const TaskPlatDescription& task_info
        ) {
    if (task_info.data_ != NULL) {
        task_info.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }
    data_ = task_info.data_;
    return (*this);
}

void TaskPlatDescription::ValueSerialization(
        base_logic::DictionaryValue* dict
        ) {
    dict->GetBigInteger(L"id", &data_->id_);
    dict->GetCharInteger(L"depth", &data_->depth_);
    dict->GetCharInteger(L"machine", &data_->machine_);
    dict->GetCharInteger(L"storage", &data_->storage_);
    dict->GetCharInteger(L"isforge", &data_->forge_);
    dict->GetCharInteger(L"isover", &data_->over_);
/*    dict->GetInteger(L"depth", reinterpret_cast<int32*>(&data_->depth_));
    dict->GetInteger(L"machine", reinterpret_cast<int32*>(&data_->machine_));
    dict->GetInteger(L"storge", reinterpret_cast<int32*>(&data_->storage_));
    dict->GetInteger(L"forge", reinterpret_cast<int32*>(&data_->forge_));
    dict->GetInteger(L"over", reinterpret_cast<int32*>(&data_->over_));
    */
    dict->GetString(L"description", &data_->description_);
}


StorageInfo::StorageInfo() {
    data_ = new Data();
}

StorageInfo::StorageInfo(const StorageInfo& info)
:data_(info.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

StorageInfo& StorageInfo::operator =(const StorageInfo& info) {
    if (info.data_ != NULL) {
        info.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }
    data_ = info.data_;
    return (*this);
}

void StorageInfo::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &data_->id_);
    dict->GetBigInteger(L"taskid", &data_->task_id_);
    int8 temp_depth = 0;
    /*dict->GetInteger(L"max_depth", reinterpret_cast<int32*>(&temp_depth));
    data_->max_depth_ = temp_depth;

    int8 cur_temp_depth = 0;
    dict->GetInteger(L"cur_depth", reinterpret_cast<int32*>(&cur_temp_depth));
    data_->cur_depth_ = cur_temp_depth;*/

    dict->GetCharInteger(L"max_depth", &data_->max_depth_);
    dict->GetCharInteger(L"cur_depth", &data_->cur_depth_);
    dict->GetString(L"name", &data_->key_name_);
    dict->GetString(L"pos", &data_->pos_name_);
    dict->GetString(L"time", &data_->time_);
    dict->GetInteger(L"attrid", &data_->attr_id_);
}


StorageHBase::StorageHBase() {
    data_ = new Data();
}

StorageHBase::StorageHBase(const StorageHBase& hbase)
:data_(hbase.data_) {
    if (data_ != NULL) {
        data_->AddRef();
    }
}

StorageHBase& StorageHBase::operator =(const StorageHBase& hbase) {
    if (hbase.data_ != NULL) {
        hbase.data_->AddRef();
    }
    if (data_ != NULL) {
        data_->Release();
    }
    data_ = hbase.data_;
    return (*this);
}

void StorageHBase::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &data_->id_);
    dict->GetBigInteger(L"taskid", &data_->task_id_);
    int8 temp_depth = 0;
    /*dict->GetInteger(L"max_depth", reinterpret_cast<int32*>(&temp_depth));
    data_->max_depth_ = temp_depth;

    int8 cur_temp_depth = 0;
    dict->GetInteger(L"cur_depth", reinterpret_cast<int32*>(&cur_temp_depth));
    data_->cur_depth_ = cur_temp_depth;*/

    dict->GetCharInteger(L"max_depth", &data_->max_depth_);
    dict->GetCharInteger(L"cur_depth", &data_->cur_depth_);
    dict->GetString(L"name", &data_->name_);
    dict->GetString(L"hkey", &data_->hkey_);
    dict->GetString(L"time", &data_->time_);
    dict->GetInteger(L"attrid", &data_->attr_id_);
}

LoginCookie::LoginCookie() {
    data_ = new Data();
}

LoginCookie::LoginCookie(const LoginCookie& login_cookie)
:data_(login_cookie.data_) {
    if (data_ != NULL)
        data_->AddRef();
}

LoginCookie& LoginCookie::operator=(const LoginCookie& login_cookie) {
    if (login_cookie.data_ != NULL)
        login_cookie.data_->AddRef();
    if (data_ != NULL)
        data_->Release();
    data_ = login_cookie.data_;
    return (*this);
}

void LoginCookie::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"cookie_id", &data_->cookie_id_);
    dict->GetBigInteger(L"cookie_attr_id", &data_->cookie_attr_id_);
    dict->GetBigInteger(L"last_time", &data_->update_last_time_);
    dict->GetString(L"cookie_body", &data_->cookie_body);
    dict->GetString(L"username", &data_->username);
    dict->GetString(L"passwd", &data_->passwd);
}

void RobotTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	std::string str;
	dict->GetString(L"timestamp", &str);
	base::BasicUtil::StringUtil::StringToInt64(str, &create_time_);
}

void RobotTask::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &id_);
}

std::string RobotTask::SerializeSelf() {
	std::stringstream os;
	os << "id: " << id_ << ", type:" << type_ << ", state: " << state_;
	return os.str();
}

void WeiboTask::GetDataFromKafka(base_logic::DictionaryValue *dict) {
	dict->GetString(L"topicId", &topic_id_);
	dict->GetString(L"hostUin", &host_uin_);
	dict->GetString(L"content", &content_);
}

std::string WeiboTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_ << ", host_uin: " << host_uin_ <<
			", content: " << content_ << ", addr: " << addr_;
	return os.str();
}

void QZoneTask::GetDataFromKafka(base_logic::DictionaryValue *dict) {
//	id_ = base::SysRadom::GetInstance()->GetRandomIntID();
	dict->GetString(L"topicId", &topic_id_);
	dict->GetBigInteger(L"hostUin", &host_uin_);
	dict->GetString(L"content", &content_);
}

std::string QZoneTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_ << ", host_uin: " << host_uin_ <<
			", content: " << content_;
	return os.str();
}

void TianyaTask::GetDataFromKafka(base_logic::DictionaryValue *dict) {
//	id_ = base::SysRadom::GetInstance()->GetRandomIntID();
	dict->GetString(L"preUrl", &url_);
	dict->GetString(L"preTitle", &title_);
	dict->GetString(L"preUserId", &user_id_);
	dict->GetString(L"preUserName", &username_);
	dict->GetBigInteger(L"prePostTime", &post_time_);
	dict->GetString(L"content", &content_);
}

std::string TianyaTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", url: " << url_ << ", title: " << title_ <<
			", user_id: " << user_id_ << ", username: " << username_ <<
			", post_time: " << post_time_ << ", content: " << content_ <<
			", addr: " << addr_;
	return os.str();
}

TiebaTask::TypeQueue TiebaTask::type_queue_;

void TiebaTask::GetDataFromKafka(base_logic::DictionaryValue *dict) {
//	id_ = base::SysRadom::GetInstance()->GetRandomIntID();
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"url", &url_);
	dict->GetString(L"cookie", &cookie_);
	dict->GetString(L"id", &row_key_);
	std::string str;
	dict->GetString(L"attrid", &str);
	type_ = atoll(str.c_str());
//	int64 task_type;
//	dict->GetBigInteger(L"attrid", &task_type);
//	type_ = (TaskType)task_type;
//	LOG_DEBUG2("str_task_type = %s, task_type = %lld",
//			str.c_str(), type_);
}

std::string TiebaTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", url: " << url_ << ", addr: " << addr_ << ",ua: " << ua_;
	return os.str();
}

void TiebaTask::ValueSerialization(base_logic::DictionaryValue *dict) {
	RobotTask::ValueSerialization(dict);
	dict->GetString(L"url", &url_);
	dict->GetBigInteger(L"attrid", &type_);
	row_key_ = url_;
}
}  // namespace base_logic
