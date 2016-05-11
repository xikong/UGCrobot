//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry
#include "auto_crawler_infos.h"
#include "basic/radom_in.h"
#include "net/comm_head.h"
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
	set_recv_last_time();
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

void RobotTaskContent::ValueSerialization(base_logic::DictionaryValue* dict) {
	dict->GetBigInteger(L"id", &id_);
	dict->GetShortInteger(L"task_type", &task_type_);
	dict->GetShortInteger(L"user_type", &user_type_);
	dict->GetString(L"content", &content_);
	LOG_DEBUG2("robot task content: %s", content_.c_str());
}

void RobotTask::GetTaskId(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"id", &id_);
}

void RobotTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	std::string str;
	dict->GetString(L"timestamp", &str);
	base::BasicUtil::StringUtil::StringToInt64(str, &create_time_);
	create_time_ /= 1000;
}

void RobotTask::GetDataFromDb(base_logic::DictionaryValue* dict) {

}

void RobotTask::SetTaskPacketUnit(RobotTaskBase *task) {
	task->task_id = id_;
	task->task_type = type_;
	task->cookie_id = cookie_.cookie_id();
	task->cookie = cookie_.get_cookie_body();
	task->ua = ua_.ua();
	task->addr = ip_.ip();
	task->content = content_.content();
}

std::string RobotTask::SerializeSelf() {
	std::stringstream os;
	os << "type:" << type_ << ", url: " << url_;
	return os.str();
}

void TiebaTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"preUrl", &url_);
	dict->GetString(L"kw", &kw_);
	dict->GetString(L"fid", &fid_);
	dict->GetString(L"tbs", &tbs_);
	std::string tmp;
	int64 i64;
	dict->GetString(L"floor_num", &tmp);
	base::BasicUtil::StringUtil::StringToInt64(tmp, &i64);
	floor_num_ = i64;
	dict->GetString(L"repostid", &repost_id_);
}

void TiebaTask::GetDataFromDb(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromDb(dict);
}

RobotTaskBase* TiebaTask::CreateTaskPacketUnit() {
	::TiebaTask *tieba_task = new ::TiebaTask();
	RobotTask::SetTaskPacketUnit(tieba_task);
	tieba_task->pre_url = url_;
	tieba_task->kw = kw_;
	tieba_task->fid = fid_;
	tieba_task->tbs = tbs_;
	tieba_task->floor_num = floor_num_;
	tieba_task->repost_id = repost_id_;
	return tieba_task;
}

void TiebaTask::SetTaskPacketUnit(RobotTaskBase *task) {
	RobotTask::SetTaskPacketUnit(task);
	// TODO
}

std::string TiebaTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", repost_id: " << repost_id_;
	return os.str();

}

RobotTask* RobotTaskFactory::Create(RobotTask::TaskType type) {
	switch(type) {
//	case RobotTask::TIANYA:
//		return new TianyaTask();
	case RobotTask::TIEBA:
		return new TiebaTask();
	case RobotTask::TAOGUBA:
		return new Taoguba();
	default:
		return NULL;
	}
}

}  // namespace base_logic
