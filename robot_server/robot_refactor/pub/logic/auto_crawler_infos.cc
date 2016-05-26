//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#include "auto_crawler_infos.h"

#include <list>
#include <sstream>

#include "../../../library/public/basic/basic_util.h"
#include "../net/comm_head.h"

namespace base_logic {

time_t TiebaTask::next_exec_time = 0;
int TiebaTask::TICK = 0;

time_t WeiboTask::next_exec_time = 0;
int WeiboTask::TICK = 0;

time_t TianyaTask::next_exec_time = 0;
int TianyaTask::TICK = 0;

time_t QZoneTask::next_exec_time = 0;
int QZoneTask::TICK = 0;

time_t MaopuTaskInfo::next_exec_time = 0;
int MaopuTaskInfo::TICK = 0;

time_t DoubanTaskInfo::next_exec_time = 0;
int DoubanTaskInfo::TICK = 0;

time_t Taoguba::next_exec_time = 0;
int Taoguba::TICK = 0;

time_t SnowballTaskInfo::next_exec_time = 0;
int SnowballTaskInfo::TICK = 0;


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
    return t_info.access_time() < r_info.access_time();
}

void ForgeryIP::ValueSerialization(base_logic::DictionaryValue* dict) {
	int64 tmp;
    dict->GetBigInteger(L"id", &tmp);
    data_->id_ = tmp;
    //  dict->GetInteger(L"type", reinterpret_cast<int32*>(&data_->type_));
    dict->GetCharInteger(L"type", &data_->type_);
    dict->GetString(L"ip", &data_->ip_);
    dict->GetString(L"create_time", &data_->create_time_);
    dict->GetBigInteger(L"access_time", &data_->access_time_);
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
	int64 tmp;
    dict->GetBigInteger(L"id", &tmp);
    data_->id_ = tmp;
    //  dict->GetInteger(L"type", reinterpret_cast<int32*>(&data_->type_));
    dict->GetCharInteger(L"type", &data_->type_);
    dict->GetString(L"ua", &data_->ua_);
    dict->GetString(L"create_time", &data_->create_time_);
    dict->GetBigInteger(L"access_time", &data_->access_time_);
}

LoginCookie::LoginCookie() {
    data_ = new Data();
}

LoginCookie::LoginCookie(const LoginCookie& login_cookie)
:data_(login_cookie.data_) {
    if (data_ != NULL)
        data_->AddRef();
    ip_ = login_cookie.ip_;
    ua_ = login_cookie.ua_;
}

LoginCookie& LoginCookie::operator=(const LoginCookie& login_cookie) {
    if (login_cookie.data_ != NULL)
        login_cookie.data_->AddRef();
    if (data_ != NULL)
        data_->Release();
    data_ = login_cookie.data_;
    ip_ = login_cookie.ip_;
    ua_ = login_cookie.ua_;
    return (*this);
}

void LoginCookie::ValueSerialization(base_logic::DictionaryValue* dict) {
    dict->GetBigInteger(L"cookie_id", &data_->cookie_id_);
    dict->GetBigInteger(L"cookie_attr_id", &data_->cookie_attr_id_);
    dict->GetBigInteger(L"last_time", &data_->last_use_time_);
    dict->GetString(L"cookie_body", &data_->cookie_body);
    dict->GetString(L"username", &data_->username);
    dict->GetString(L"passwd", &data_->passwd);
    int64 tmp;
    std::string tmp_string;
    dict->GetBigInteger(L"ip_id", &tmp);
    ip_.set_id(tmp);
    dict->GetBigInteger(L"ua_id", &tmp);
    ua_.set_id(tmp);
    dict->GetString(L"ip", &tmp_string);
    ip_.set_ip(tmp_string);
    LOG_DEBUG2("cookie_id = %lld, ip_id = %d, ip = %s, ua_id = %d",
    		cookie_id(), ip_.id(), ip_.ip().c_str(), ua_.id());
}

RobotTaskContent::RobotTaskContent(const RobotTaskContent& other) {
	id_ = other.id();
	task_type_ = other.task_type();
	user_type_ = other.user_type();
	content_ = other.content();
}

RobotTaskContent& RobotTaskContent::operator=(const RobotTaskContent& other) {
	id_ = other.id();
	task_type_ = other.task_type();
	user_type_ = other.user_type();
	content_ = other.content();
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
//	std::string str;
//	dict->GetString(L"timestamp", &str);
//	base::BasicUtil::StringUtil::StringToInt64(str, &create_time_);
//	create_time_ /= 1000;
	create_time_ = time(NULL);
}

void RobotTask::GetDataFromDb(base_logic::DictionaryValue* dict) {

}

void RobotTask::SetTaskPacketUnit(RobotTaskBase *task) {
	task->task_id = id_;
	task->task_type = type_;
	task->cookie_id = cookie_.cookie_id();
	task->cookie = cookie_.get_cookie_body();
	task->ua = cookie_.ua_.ua();
	task->addr = cookie_.ip_.ip();
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

void Taoguba::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"id", &topic_id_);
	dict->GetString(L"title", &subject_);
}

void Taoguba::GetDataFromDb(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromDb(dict);
}

RobotTaskBase* Taoguba::CreateTaskPacketUnit() {
	::TaogubaTask *taoguba_task = new ::TaogubaTask();
	RobotTask::SetTaskPacketUnit(taoguba_task);
	taoguba_task->topic_id = topic_id_;
	taoguba_task->subject = subject_;
	return taoguba_task;
}

void Taoguba::SetTaskPacketUnit(RobotTaskBase *task) {
	RobotTask::SetTaskPacketUnit(task);
	// TODO
}

std::string Taoguba::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_ << ", subject: " << subject_;
	return os.str();
}

void WeiboTask::GetTaskId(base_logic::DictionaryValue* dict) {
	RobotTask::GetTaskId(dict);
}

void WeiboTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"topicId", &topic_id_);
	dict->GetString(L"hostUin", &host_uin_);
}

void WeiboTask::GetDataFromDb(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromDb(dict);
}

RobotTaskBase* WeiboTask::CreateTaskPacketUnit() {
	::WeiBoTask *weibo_task = new ::WeiBoTask();
	RobotTask::SetTaskPacketUnit(weibo_task);
	weibo_task->topic_id = topic_id_;
	weibo_task->host_uin = host_uin_;
	return weibo_task;
}

void WeiboTask::SetTaskPacketUnit(RobotTaskBase *task) {
	RobotTask::SetTaskPacketUnit(task);
}

std::string WeiboTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_ << ", host_uin: " << host_uin_;
	return os.str();
}

void TianyaTask::GetTaskId(base_logic::DictionaryValue* dict) {
	RobotTask::GetTaskId(dict);
}

void TianyaTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"preUrl", &url_);
	dict->GetString(L"preTitle", &title_);
	dict->GetString(L"preUserId", &user_id_);
	dict->GetString(L"preUserName", &username_);
	dict->GetBigInteger(L"prePostTime", &post_time_);
}

void TianyaTask::GetDataFromDb(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromDb(dict);
}

RobotTaskBase* TianyaTask::CreateTaskPacketUnit() {
	::TianyaTask *tianya_task = new ::TianyaTask();
	RobotTask::SetTaskPacketUnit(tianya_task);
	tianya_task->pre_post_time = post_time_;
	tianya_task->pre_url = url_;
	tianya_task->pre_title = title_;
	tianya_task->pre_user_id = user_id_;
	tianya_task->pre_user_name = username_;
	return tianya_task;
}

void TianyaTask::SetTaskPacketUnit(RobotTaskBase *task) {
	RobotTask::SetTaskPacketUnit(task);
}

std::string TianyaTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", title: " << title_ << ", user_id: " << user_id_
			<< ", user_name: " << username_;
	return os.str();
}

void QZoneTask::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"topicId", &topic_id_);
	dict->GetBigInteger(L"hostUin", &host_uin_);
}

RobotTaskBase* QZoneTask::CreateTaskPacketUnit() {
	::QzoneTask *qzone_task = new ::QzoneTask();
	RobotTask::SetTaskPacketUnit(qzone_task);
	qzone_task->host_uin = host_uin_;
	qzone_task->topic_id = topic_id_;
	return qzone_task;
}

std::string QZoneTask::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_ << ", host_uin: " << host_uin_;
	return os.str();
}

void MaopuTaskInfo::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"pre_url", &url_);
	dict->GetString(L"cat_id", &cat_id_);
	dict->GetString(L"catalog_id", &catalog_id_);
	dict->GetString(L"fmtoken", &fmtoken_);
	dict->GetString(L"currformid", &currformid_);
}

RobotTaskBase* MaopuTaskInfo::CreateTaskPacketUnit() {
	::MaopuTask *task = new ::MaopuTask();
	RobotTask::SetTaskPacketUnit(task);
	task->pre_url = url_;
	task->cat_id = cat_id_;
	task->catalog_id = catalog_id_;
	task->fmtoken = fmtoken_;
	task->currformid = currformid_;
	return task;
}

std::string MaopuTaskInfo::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", cat_id: " << cat_id_ << ", catalog_id: " << catalog_id_
			<< ", fmtoken: " << fmtoken_ << ", currformid: " << currformid_;
	return os.str();
}

void DoubanTaskInfo::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"pre_url", &url_);
}

RobotTaskBase* DoubanTaskInfo::CreateTaskPacketUnit() {
	::Douban *task = new ::Douban();
	RobotTask::SetTaskPacketUnit(task);
	task->pre_url = url_;
	return task;
}

std::string DoubanTaskInfo::SerializeSelf() {
	return RobotTask::SerializeSelf();
}

void SnowballTaskInfo::GetDataFromKafka(base_logic::DictionaryValue* dict) {
	RobotTask::GetDataFromKafka(dict);
	dict->GetString(L"url", &url_);
	dict->GetString(L"topicId", &topic_id_);
}

RobotTaskBase* SnowballTaskInfo::CreateTaskPacketUnit() {
	SnowBall *task = new SnowBall();
	RobotTask::SetTaskPacketUnit(task);
	task->pre_url = url_;
	task->topic_id = topic_id_;
	return task;
}

std::string SnowballTaskInfo::SerializeSelf() {
	std::stringstream os;
	os << RobotTask::SerializeSelf();
	os << ", topic_id: " << topic_id_;
	return os.str();
}

RobotTask* RobotTaskFactory::Create(RobotTask::TaskType type) {
	switch(type) {
	case RobotTask::TIEBA:
		return new TiebaTask();
	case RobotTask::TAOGUBA:
		return new Taoguba();
	case RobotTask::WEIBO:
		return new WeiboTask();
	case RobotTask::TIANYA:
		return new TianyaTask();
	case RobotTask::QZONE:
		return new QZoneTask();
	case RobotTask::MAOPU:
		return new MaopuTaskInfo();
	case RobotTask::DOUBAN:
		return new DoubanTaskInfo();
	case RobotTask::SNOWBALL:
		return new SnowballTaskInfo();
	default:
		return NULL;
	}
}

}  // namespace base_logic
