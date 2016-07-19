//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#ifndef AUTO_CRAWLER_INFOS_H_
#define AUTO_CRAWLER_INFOS_H_

#include <string>
#include <map>

#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "logic/logic_comm.h"

struct RobotTaskBase;
struct RouterStatus;

enum TaskState{
    TASK_WAIT = 0,
    TASK_SEND = 1,
    TASK_RECV = 2,
    TASK_READY = 3,
    TASK_EXECUING = 4,
    TASK_STORAGE = 5,
    TASK_STORAGED = 6,
    TASK_EXECUED = 7,
    TASK_FAIL = 8,
    TASK_SUCCESS = 9,
    TASK_TIMEOUT = 10,
    TASK_SEND_FAILED = 11,
    TASK_INVALID = 12,
    TASK_NO_RESPNOSE = 13
};

enum ANALYTICALSTATE{
    ANALYTICAL_WAIT = 0,
    ANALYTICAL_EXECUTING = 1,
    ANALYTICAL_EXECUED = 2
};

enum CRAWLERTYPE{
    STORAGE_REDIS = 0,
    STORAGE_HBASE = 1,
    STORAGE_MYSQL = 2,
    STORAGE_TEXT = 3,
    STORAGE_MEMCACHE = 4
};

namespace base_logic {

class ForgeryIP{
 public:
    ForgeryIP();
    ForgeryIP(const ForgeryIP& ip );

    ForgeryIP& operator =(const ForgeryIP& ip );

    ~ForgeryIP() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void update_time() {
        data_->send_last_time_ = time(NULL);
    }

    void update_send_time(time_t appoint_time ) {
        data_->send_last_time_ = appoint_time;
    }

    const time_t send_last_time() const {
        return data_->send_last_time_;
    }

    void set_id(const int32 id ) {
        data_->id_ = id;
    }
    void set_type(const int8 type ) {
        data_->type_ = type;
    }
    void set_create_time(const std::string& create_time ) {
        data_->create_time_ = create_time;
    }
    void set_ip(const std::string& ip ) {
        data_->ip_ = ip;
    }

    void Addcount() {
        __sync_fetch_and_add(&data_->count_, 1);
    }

    void Refcount() {
        __sync_fetch_and_sub(&data_->count_, 1);
    }

    const int32 id() const {
        return data_->id_;
    }
    const int8 type() const {
        return data_->type_;
    }
    const int64 count() const {
        return data_->count_;
    }
    const std::string& create_time() const {
        return data_->create_time_;
    }
    const std::string& ip() const {
        return data_->ip_;
    }

    void update_access_time(time_t t = time(NULL) ) {
        data_->access_time_ = t;
    }
    time_t access_time() const {
        return data_->access_time_;
    }

    static bool cmp(const ForgeryIP& t_info, const ForgeryIP& r_info );

    void ValueSerialization(base_logic::DictionaryValue* dict );

    class Data{
     public:
        Data()
                : refcount_(1),
                  id_(0),
                  type_(0),
                  count_(0),
                  send_last_time_(0),
                  access_time_(0) {
        }

     public:
        int32 id_;
        int8 type_;
        int64 count_;
        time_t send_last_time_;
        time_t access_time_;
        std::string create_time_;
        std::string ip_;
        void AddRef() {
            __sync_fetch_and_add(&refcount_, 1);
        }

        void Release() {
            __sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_)
                delete this;
        }
     private:
        int refcount_;
    };

    Data* data_;
};

class ForgeryUA{
 public:
    ForgeryUA();
    ForgeryUA(const ForgeryUA& ua );

    ForgeryUA& operator =(const ForgeryUA& ua );

    static bool cmp(const ForgeryUA& t_info, const ForgeryUA& r_info ) {
        t_info.access_time() < r_info.access_time();
    }

    ~ForgeryUA() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int32 id ) {
        data_->id_ = id;
    }
    void set_type(const int8 type ) {
        data_->type_ = type;
    }
    void set_create_time(const std::string& create_time ) {
        data_->create_time_ = create_time;
    }
    void set_ua(const std::string& ua ) {
        data_->ua_ = ua;
    }

    void update_send_time() {
        data_->send_time_ = time(NULL);
    }

    time_t send_time() const {
        return data_->send_time_;
    }

    void Addcount() {
        __sync_fetch_and_add(&data_->count_, 1);
    }

    void Refcount() {
        __sync_fetch_and_sub(&data_->count_, 1);
    }

    const int32 id() const {
        return data_->id_;
    }
    const int8 type() const {
        return data_->type_;
    }
    const std::string& create_time() const {
        return data_->create_time_;
    }
    const std::string& ua() const {
        return data_->ua_;
    }
    const int64 count() const {
        return data_->count_;
    }

    void update_access_time(time_t t = time(NULL) ) {
        data_->access_time_ = t;
    }
    time_t access_time() const {
        return data_->access_time_;
    }

    void ValueSerialization(base_logic::DictionaryValue* dict );

    class Data{
     public:
        Data()
                : refcount_(1),
                  id_(0),
                  type_(0),
                  count_(0),
                  send_time_(time(NULL)),
                  access_time_(0) {
        }

     public:
        int32 id_;
        int8 type_;
        int64 count_;
        time_t send_time_;
        time_t access_time_;
        std::string create_time_;
        std::string ua_;
        void AddRef() {
            __sync_fetch_and_add(&refcount_, 1);
        }

        void Release() {
            __sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_)
                delete this;
        }
     private:
        int refcount_;
    };

    Data* data_;
};

class MailContentInfo{
 public:
    MailContentInfo()
            : id_(-1) {

    }
    MailContentInfo(const MailContentInfo &other );
    MailContentInfo & operator=(const MailContentInfo &other );
    void ValueSerialize(base_logic::DictionaryValue *dict );

 public:
    int64 id_;
    std::string mailsubject_;
    std::string mailbody_;
};

class MailTargetInfo{
 public:
    void SetMailTarget(const std::string &target ) {
        target_ = target;
    }
    const std::string GetMailTarget() const {
        return target_;
    }

    void GetDataFromKafka(base_logic::DictionaryValue &dic );

 private:
    std::string target_;
};

template<typename T>
class SmartPtr;

template<typename T>
class WrapData{
    friend class SmartPtr<T> ;
 private:
    WrapData(T *p )
            : data_(p),
              refcount_(1) {
    }
    ~WrapData() {
        delete data_;
    }
 private:
    int refcount_;
    T *data_;
};

template<typename T>
class SmartPtr{
 public:
    SmartPtr(T *ptr )
            : p_(new WrapData<T>(ptr)) {
    }
    SmartPtr(const SmartPtr<T> &sp )
            : p_(sp.p_) {
        if (NULL != p_)
            AddRef();
    }
    ~SmartPtr() {
        if (NULL != p_)
            Release();
    }
    SmartPtr<T>& operator=(const SmartPtr<T> &sp ) {
        if (NULL != sp.p_)
            sp->AddRef();
        if (NULL != p_)
            Release();
        p_ = sp.p_;
    }

    T& operator*() {
        return *p_->data_;
    }

    T* operator->() {
        return p_->data_;
    }
 private:
    void AddRef() {
        __sync_fetch_and_add(&p_->refcount_, 1);
    }

    void Release() {
        __sync_fetch_and_sub(&p_->refcount_, 1);
        if (!p_->refcount_)
            delete p_;
    }
 private:
    WrapData<T> *p_;
};

}  // namespace base_logic

#endif /* CRAWLER_MANGER_INFOS_H_ */
