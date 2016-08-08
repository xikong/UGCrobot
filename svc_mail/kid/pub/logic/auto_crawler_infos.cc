//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#include "auto_crawler_infos.h"

#include <list>
#include <sstream>

#include "../../../library/public/basic/basic_util.h"
#include "../net/comm_head.h"

namespace base_logic {

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

MailContentInfo::MailContentInfo(const MailContentInfo &other){
    id_ = other.id_;
    mailsubject_ = other.mailsubject_;
    mailbody_ = other.mailbody_;
}

MailContentInfo & MailContentInfo::operator=(const MailContentInfo &other){
    if( this == &other){
        return *this;
    }
    id_ = other.id_;
    mailsubject_ = other.mailsubject_;
    mailbody_ = other.mailbody_;
    return *this;
}

void MailContentInfo::ValueSerialize(base_logic::DictionaryValue *dict){
    dict->GetBigInteger(L"id", &id_);
    dict->GetString(L"mail_subject", &mailsubject_);
    dict->GetString(L"mail_body", &mailbody_);

    LOG_DEBUG2("LoadDb MailInfo Success subject = %s, body = %s", mailsubject_.c_str(), mailbody_.c_str());
}

void MailTargetInfo::GetDataFromKafka(base_logic::DictionaryValue &dic){
    dic.GetString(L"mail_target", &target_);
}

}  // namespace base_logic
