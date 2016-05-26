//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#ifndef AUTO_CRAWLER_INFOS_H_
#define AUTO_CRAWLER_INFOS_H_

#include <string>
#include <map>

#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "logic/logic_comm.h"
#include "net/comm_head.h"

enum TASKSTAE {
  TASK_WAIT = 0,
  TASK_SEND = 1,
  TASK_RECV = 2,
  TASK_READY = 3,
  TASK_EXECUING = 4,
  TASK_STORAGE = 5,
  TASK_STORAGED = 6,
  TASK_EXECUED = 7
};

enum TASKTYPE {
  UNKNOW_TASK = 0,
  MAIN_LASTING_TASK = 1,
  MAIN_SHORT_TASK = 2,
  TEMP_LASTING_TASK = 3,
  TEMP_SHORT_TASK = 4,
  NEWS_DETAIL_TASK = 5,
  WEIBO_TASK = 6
};

enum ANALYTICALSTATE {
  ANALYTICAL_WAIT = 0,
  ANALYTICAL_EXECUTING = 1,
  ANALYTICAL_EXECUED = 2
};

enum CRAWLERTYPE {
  STORAGE_REDIS = 0,
  STORAGE_HBASE = 1,
  STORAGE_MYSQL = 2,
  STORAGE_TEXT = 3,
  STORAGE_MEMCACHE = 4
};

namespace base_logic {

template<typename T>
class WrapData {
  friend class SmartPtr<T> ;
 private:
  WrapData(T *p)
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
class SmartPtr {
 public:
  SmartPtr(T *ptr)
      : p_(new WrapPtr<T>(ptr)) {
  }
  SmartPtr(const SmartPtr<T> &sp)
      : p_(sp.p_) {
    if (NULL != p_)
      AddRef();
  }
  ~SmartPtr() {
    if (NULL != p_)
      Release();
  }
  SmartPtr<T>& operator=(const SmartPtr<T> &sp) {
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
      delete wp_;
  }
 private:
  WrapData<T> *p_;
};

class RobotTask {
 public:
  enum TaskType {
    UNKNOWN,
    WEIBO,
    QZONE
  };
  RobotTask()
      : id_(0),
        type_(UNKNOWN) {
  }
  virtual void GetDataFromKafka(base_logic::DictionaryValue* dict) {
  }
  virtual void GetDataFromDb(base_logic::DictionaryValue* dict) {
  }

  void set_id(int64 id) {
    id_ = id;
  }
  int64 id() const {
    return id_;
  }
  TaskType type() const {
    return type_;
  }
 protected:
  int64 id_;		//任务 id
  TaskType type_;
};

class QZoneTask : public RobotTask {
 public:
  QZoneTask() {
  }
  virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);

 public:
  void set_addr(const std::string &addr) {
    addr_ = addr;
  }
  std::string addr() const {
    return addr_;
  }
 private:
  std::string topic_id_;		//留言对象
  std::string host_uin_;		//对方的 qq 号
  std::string uin_;			//自己的 qq 号
  std::string contenet_;		//回复内容
  std::string addr_;
};
}  // namespace base_logic

#endif /* CRAWLER_MANGER_INFOS_H_ */
