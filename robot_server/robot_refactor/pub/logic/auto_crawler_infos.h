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

enum TaskState {
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
	TASK_SEND_FAILED= 11,
	TASK_INVALID = 12
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

class CrawlerScheduler {
public:
   CrawlerScheduler();

   CrawlerScheduler(const CrawlerScheduler& crawler_scheduler);

   CrawlerScheduler& operator = (const CrawlerScheduler& crawler_scheduler);

   ~CrawlerScheduler() {
       if (data_ != NULL) {
           data_->Release();
       }
   }

   void set_id(const int32 id) {data_->id_ = id;}
   void set_socket(const int socket) {data_->socket_ = socket;}
   void set_ip(const std::string& ip) {data_->ip_ = ip;}
   void set_idle_tasks(uint16 crawler_type, const int32 task_count) {
       data_->task_count_ = task_count;
   }

   void set_mac(const std::string& mac) {data_->mac_ = mac;}
   void set_password(const std::string& password) {
       data_->password_ = password;
   }

   void set_recv_last_time(const time_t recv_last_time) {
       data_->recv_last_time_ = recv_last_time;
   }

   void set_send_last_time(const time_t send_last_time) {
       data_->send_last_time_ = send_last_time;
   }

   void set_is_effective(bool is_effective) {
       data_->is_effective_ = is_effective;
   }

   void add_send_error_count() {
       __sync_fetch_and_add(&data_->send_error_count_, 1);
   }

   void add_recv_error_count() {
       __sync_fetch_and_add(&data_->recv_error_count_, 1);
   }
   const int32 id() const {return data_->id_;}
   const int task_count() const {return data_->task_count_;}
   const int socket() const {return data_->socket_;}
   const time_t send_last_time() const {return data_->send_last_time_;}
   const time_t recv_last_time() const {return data_->recv_last_time_;}
   const int32 send_error_count() const {return data_->send_error_count_;}
   const bool is_effective() const {return data_->is_effective_;}
   const std::string& ip() const {return data_->ip_;}
   const std::string& mac() const {return data_->mac_;}
   const std::string& password() const {return data_->password_;}

   static inline bool cmp(const CrawlerScheduler& t_scheduler,
           const CrawlerScheduler& r_scheduler) {
       return t_scheduler.task_count() > r_scheduler.task_count();
   }


private:
   class Data{
    public:
       Data()
       :refcount_(1)
       , id_(0)
       , is_effective_(true)
       , task_count_(0)
       , socket_(0)
       , send_error_count_(0)
       , recv_error_count_(0)
       , send_last_time_(0)
       , recv_last_time_(0) {}

    public:
       int32       id_;
       int         task_count_;
       int32       send_error_count_;
       int32       recv_error_count_;
       bool        is_effective_;
       time_t      send_last_time_;
       time_t      recv_last_time_;
       int         socket_;
       std::string ip_;
       std::string password_;
       std::string mac_;
       void AddRef() {__sync_fetch_and_add(&refcount_, 1);}
       void Release() {__sync_fetch_and_sub(&refcount_, 1);
           if (!refcount_)delete this;
       }

    private:
       int         refcount_;
   };
   Data*       data_;
};

class RouterScheduler {
public:
   typedef std::map<uint16, uint32>		CrawlerTypeTasksSumMap;
   RouterScheduler();

   RouterScheduler(const RouterScheduler& router_scheduler);

   RouterScheduler& operator = (const RouterScheduler& router_scheduler);

   ~RouterScheduler() {
       if (data_ != NULL) {
           data_->Release();
       }
   }

   void set_id(const int32 id) {data_->id_ = id;}
   void set_socket(const int socket) {data_->socket_ = socket;}
   void set_ip(const std::string& ip) {data_->ip_ = ip;}
   void set_idle_tasks(uint16 crawler_type, const int32 task_count) {
	  data_->crawler_type_tasks_sum_map_[crawler_type] = task_count;
   }

   void set_mac(const std::string& mac) {data_->mac_ = mac;}
   void set_password(const std::string& password) {
       data_->password_ = password;
   }

   void set_recv_last_time(const time_t recv_last_time = time(NULL)) {
       data_->recv_last_time_ = recv_last_time;
   }

   void set_send_last_time(const time_t send_last_time) {
       data_->send_last_time_ = send_last_time;
   }

   void set_is_effective(bool is_effective) {
       data_->is_effective_ = is_effective;
   }

	void set_router_status(struct RouterStatus *router_status, int socket);

   void add_send_error_count() {
       __sync_fetch_and_add(&data_->send_error_count_, 1);
   }

   void add_recv_error_count() {
       __sync_fetch_and_add(&data_->recv_error_count_, 1);
   }
   const int32 id() const {return data_->id_;}
   const int idle_tasks(uint16 crawler_type) const {
	  if (!has_crawler_type(crawler_type)) {
		  LOG_DEBUG2("can't find crawler: %d, data_->crawler_type_tasks_sum_map_[crawler_type] = %d", crawler_type, data_->crawler_type_tasks_sum_map_[crawler_type]);
		  return 0;
	  }
	  return data_->crawler_type_tasks_sum_map_[crawler_type];
	}
   const int socket() const {return data_->socket_;}
   const time_t send_last_time() const {return data_->send_last_time_;}
   const time_t recv_last_time() const {return data_->recv_last_time_;}
   const int32 send_error_count() const {return data_->send_error_count_;}
   const bool is_effective() const {return data_->is_effective_;}
	const bool has_crawler_type(uint16 crawler_type) const {
	  return data_->crawler_type_tasks_sum_map_.find(crawler_type) != data_->crawler_type_tasks_sum_map_.end();
	}
   const std::string& ip() const {return data_->ip_;}
   const std::string& mac() const {return data_->mac_;}
   const std::string& password() const {return data_->password_;}
private:
   class Data{
    public:
       Data()
       :refcount_(1)
       , id_(0)
       , is_effective_(true)
       , socket_(0)
       , send_error_count_(0)
       , recv_error_count_(0)
       , send_last_time_(0)
       , recv_last_time_(0) {}

    public:
       int32       id_;
       int32       send_error_count_;
       int32       recv_error_count_;
       bool        is_effective_;
       time_t      send_last_time_;
       time_t      recv_last_time_;
       int         socket_;
       std::string ip_;
       std::string password_;
       std::string mac_;
		CrawlerTypeTasksSumMap crawler_type_tasks_sum_map_;
       void AddRef() {__sync_fetch_and_add(&refcount_, 1);}
       void Release() {__sync_fetch_and_sub(&refcount_, 1);
           if (!refcount_)delete this;
       }

    private:
       int         refcount_;
   };
   Data*       data_;
};

inline bool superior_to(const RouterScheduler& t_scheduler,
	const RouterScheduler& r_scheduler, int16 crawler_type) {
  if (!t_scheduler.has_crawler_type(crawler_type) && !r_scheduler.has_crawler_type(crawler_type)) {
	LOG_MSG2("neither router scheduler has the crawler_type: %d", crawler_type);
	return false;
  }
  if (!t_scheduler.has_crawler_type(crawler_type))
	return false;
  return t_scheduler.idle_tasks(crawler_type) > r_scheduler.idle_tasks(crawler_type);
}

class ForgeryIP {
public:
   ForgeryIP();
   ForgeryIP(const ForgeryIP& ip);

   ForgeryIP& operator = (const ForgeryIP& ip);

   ~ForgeryIP() {
       if (data_ != NULL) {
           data_->Release();
       }
   }

   void update_time() {
       data_->send_last_time_ = time(NULL);
   }

   void update_send_time(time_t appoint_time) {
       data_->send_last_time_ = appoint_time;
   }

   const time_t send_last_time() const {
       return data_->send_last_time_;
   }

   void set_id(const int32 id) {data_->id_ = id;}
   void set_type(const int8 type) {data_->type_ = type;}
   void set_create_time(const std::string& create_time) {
       data_->create_time_ = create_time;
   }
   void set_ip(const std::string& ip) {data_->ip_ = ip;}

   void Addcount() {
       __sync_fetch_and_add(&data_->count_, 1);
   }

   void Refcount() {
       __sync_fetch_and_sub(&data_->count_, 1);
   }



   const int32 id() const {return data_->id_;}
   const int8 type() const {return data_->type_;}
   const int64 count() const {return data_->count_;}
   const std::string& create_time() const {return data_->create_time_;}
   const std::string& ip() const {return data_->ip_;}

   void update_access_time(time_t t = time(NULL)) { data_->access_time_ = t; }
   time_t access_time() const { return data_->access_time_; }

   static bool cmp(const ForgeryIP& t_info, const ForgeryIP& r_info);

   void ValueSerialization(base_logic::DictionaryValue* dict);


   class Data {
    public:
       Data()
        : refcount_(1)
        , id_(0)
        , type_(0)
        , count_(0)
    	, send_last_time_(0)
    	, access_time_(0) {}

    public:
       int32       id_;
       int8        type_;
       int64       count_;
       time_t		send_last_time_;
       time_t		access_time_;
       std::string create_time_;
       std::string ip_;
       void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

       void Release() {__sync_fetch_and_sub(&refcount_, 1);
           if (!refcount_)delete this;
       }
    private:
       int refcount_;
   };

   Data*     data_;
};

class ForgeryUA {
public:
   ForgeryUA();
   ForgeryUA(const ForgeryUA& ua);

   ForgeryUA& operator = (const ForgeryUA& ua);

   static bool cmp(const ForgeryUA& t_info, const ForgeryUA& r_info) {
	   t_info.access_time() < r_info.access_time();
   }

   ~ForgeryUA() {
       if (data_ != NULL) {
           data_->Release();
       }
   }

   void set_id(const int32 id) {data_->id_ = id;}
   void set_type(const int8 type) {data_->type_ = type;}
   void set_create_time(const std::string& create_time) {
       data_->create_time_ = create_time;
   }
   void set_ua(const std::string& ua) {
       data_->ua_ = ua;
   }

   void update_send_time() {
	   data_->send_time_ = time(NULL);
   }

   time_t send_time() const {return data_->send_time_;}

   void Addcount() {
       __sync_fetch_and_add(&data_->count_, 1);
   }

   void Refcount() {
       __sync_fetch_and_sub(&data_->count_, 1);
   }


   const int32 id() const {return data_->id_;}
   const int8 type() const {return data_->type_;}
   const std::string& create_time() const {return data_->create_time_;}
   const std::string& ua() const {return data_->ua_;}
   const int64 count() const {return data_->count_;}

   void update_access_time(time_t t = time(NULL)) { data_->access_time_ = t; }
   time_t access_time() const { return data_->access_time_; }

   void ValueSerialization(base_logic::DictionaryValue* dict);

   class Data{
    public:
       Data()
        : refcount_(1)
        , id_(0)
        , type_(0)
        , count_(0)
    	, send_time_(time(NULL))
    	, access_time_(0) {}

    public:
       int32       id_;
       int8        type_;
       int64       count_;
       time_t	   send_time_;
       time_t		access_time_;
       std::string create_time_;
       std::string ua_;
       void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

       void Release() {__sync_fetch_and_sub(&refcount_, 1);
           if (!refcount_)delete this;
       }
    private:
       int refcount_;
   };

   Data*       data_;
};

class LoginCookie {
public:
   LoginCookie();
   LoginCookie(const LoginCookie& login_cookie);
   LoginCookie& operator=(const LoginCookie& login_cookie);
   ~LoginCookie() {
       if (data_ != NULL) {
           data_->Release();
       }
   }

   void update_time() {
       data_->send_last_time_ = time(NULL);
   }

   void set_cookie_attr_id(const int64 id) {
       data_->cookie_attr_id_ = id;
   }
   void set_cookie_body(const std::string& cookie_body) {
       data_->cookie_body = cookie_body;
   }

   void set_username(const std::string& username) {
       data_->username = username;
   }

   void set_passwd(const std::string& passwd) {
       data_->passwd = passwd;
   }

   void set_is_read(bool IsRead) {
       data_->is_read = IsRead;
   }

   bool is_over_time(time_t appoint_time) {
       if (!data_->is_first) {
           data_->is_first = true;
           return true;
       } else {
           return (data_->send_last_time_ + 3600) < appoint_time
               ? true : false;
       }
   }

   void update_send_time(time_t appoint_time) {
       data_->send_last_time_ = appoint_time;
   }

   const time_t send_last_time() const {
       return data_->send_last_time_;
   }

   const int64 get_cookie_attr_id() const {
       return data_->cookie_attr_id_;
   }
   const std::string& get_cookie_body() const {
       return data_->cookie_body;
   }

   const std::string& get_username() const {
       return data_->username;
   }

   const std::string& get_passwd() const {
       return data_->passwd;
   }

   const bool get_is_read() const {
       return data_->is_read;
   }

   const time_t get_update_time() const {
       return data_->update_last_time_;
   }

   const int64 cookie_id() const {return data_->cookie_id_;}

   static inline bool cmp(const LoginCookie& t_login_cookie,
           const LoginCookie& r_login_cookie) {
       return t_login_cookie.send_last_time() < r_login_cookie.send_last_time();
   }

   void ValueSerialization(base_logic::DictionaryValue* dict);

public:
   ForgeryIP	ip_;
   ForgeryUA	ua_;

   class Data {
    public:
           Data()
           : refcount_(1)
           , cookie_attr_id_(0)
           , cookie_id_(0)
           , is_read(false)
           , is_first(false)
           , send_last_time_(0) {
           }

    public:
           int64        cookie_id_;
           int64        cookie_attr_id_;
           time_t       send_last_time_;
           time_t       update_last_time_;
           std::string  cookie_body;
           std::string  username;
           std::string  passwd;
           bool         is_read;
           bool         is_first;
//           ForgeryIP	ip_;
//           ForgeryUA	ua_;
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

class RobotTaskContent {
public:
	void ValueSerialization(base_logic::DictionaryValue* dict);
	int64 id() const {return id_;}
	std::string content() const {return content_;}
	int16 task_type() const {return task_type_;}
	int16 user_type() const {return user_type_;}
private:
	int64		id_;
	int16		task_type_;
	int16		user_type_;
	std::string	content_;
};

template <typename T>
class SmartPtr;

template <typename T>
class WrapData {
	friend class SmartPtr<T>;
private:
	WrapData(T *p): data_(p), refcount_(1) {}
	~WrapData() {
		delete data_;
	}
private:
	int refcount_;
	T *data_;
};

template <typename T>
class SmartPtr {
public:
	SmartPtr(T *ptr): p_(new WrapData<T>(ptr)) {}
	SmartPtr(const SmartPtr<T> &sp): p_(sp.p_) {
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
			delete p_;
	}
private:
	WrapData<T> *p_;
};

class RobotTask {
public:
	enum TaskType{
		UNKNOWN,
		WEIBO,
		QZONE,
		TIEBA = 7001,
		TAOGUBA = 7007,
		TIANYA
	};

	RobotTask()
		: id_(0)
		, type_(UNKNOWN)
		, state_(TASK_WAIT)
		, create_time_(time(NULL))
		, send_time_(0) {}

	virtual void GetTaskId(base_logic::DictionaryValue* dict);
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual void GetDataFromDb(base_logic::DictionaryValue* dict);
	virtual RobotTaskBase* CreateTaskPacketUnit() = 0;
	virtual void SetTaskPacketUnit(RobotTaskBase *task);
	virtual std::string SerializeSelf();

	void set_id(int64 id) {id_ = id;}
	int64 id() const {return id_;}

	void set_type(TaskType type) {type_ = type;}
	TaskType type() const {return type_;}

	void set_state(TaskState state) {state_ = state;}
	TaskState state() const {return state_;}

	void set_url(std::string &url) {url_ = url;}
	std::string url() const {return url_;}

	void set_send_time(time_t t = time(NULL)) {send_time_ = t;}
	time_t send_time() const {return send_time_;}

	void set_create_time(time_t t = time(NULL)) {create_time_ = t;}
	time_t create_time() const {return create_time_;}

	void set_cookie(base_logic::LoginCookie &cookie) { cookie_ = cookie; }
	base_logic::LoginCookie cookie() const { return cookie_; }

	void set_ua(base_logic::ForgeryUA &ua) { ua_ = ua; }
	base_logic::ForgeryUA ua() const { return ua_; }

	void set_ip(base_logic::ForgeryIP &ip) { ip_ = ip; }
	base_logic::ForgeryIP ip() const { return ip_; }

	void set_content(base_logic::RobotTaskContent &con) { content_ = con; }
	base_logic::RobotTaskContent content() const { return content_; }

protected:
	int64		id_;		//任务 id
	TaskType	type_;
	std::string url_;
	TaskState	state_;
	time_t		create_time_;
	time_t		send_time_;
	base_logic::LoginCookie 		cookie_;
	base_logic::ForgeryUA 			ua_;
	base_logic::ForgeryIP			ip_;
	base_logic::RobotTaskContent	content_;
};

#if 0
class WeiboTask: public RobotTask {
public:
	WeiboTask() {type_ = WEIBO;}
	virtual void GetTaskId(base_logic::DictionaryValue* dict) {}
	virtual void GetDataFromDb(base_logic::DictionaryValue* dict) {}
	virtual RobotTaskBase* CreateTaskPacketUnit() {return NULL;}
	virtual void SetTaskPacketUnit(RobotTaskBase *task) {}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict) {}
	virtual std::string SerializeSelf();
	virtual RobotTaskBase* CreateTaskPacketUnit() {return NULL;}
public:
	void set_addr(const std::string &addr) {addr_ = addr;}
	std::string addr() const {return addr_;}
	void set_topic_id(const std::string &topic_id) {topic_id_ = topic_id;}
	std::string topic_id() const {return topic_id_;}
	void set_host_uin(const std::string host_uin) {host_uin_ = host_uin;}
	std::string host_uin() const {return host_uin_;}
//	void set_content(const std::string &content) {content_ = content;}
//	std::string content() const {return content_;}
private:
	std::string		topic_id_;
	std::string		host_uin_;
//	std::string		content_;
	std::string		addr_;
};

class TianyaTask: public RobotTask {
public:
	TianyaTask() {type_ = TIANYA;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
	virtual RobotTaskBase* CreateTaskPacketUnit() {return NULL;}
public:
	void set_addr(const std::string &addr) {addr_ = addr;}
	std::string addr() const {return addr_;}
	void set_post_time(int64 post_time) {post_time_ = post_time;}
	uint64 post_time() const {return post_time_;}
	void set_url(const std::string &url) {url_ = url;}
	std::string url() const {return url_;}
	void set_title(const std::string &title) {title_ = title;}
	std::string title() const {return title_;}
	void set_user_id(const std::string &user_id) {user_id_ = user_id;}
	std::string user_id() const {return user_id_;}
	void set_username(const std::string &username) {username_ = username;}
	std::string username() const {return username_;}
//	void set_content(const std::string &content) {content_ = content;}
//	std::string content() const {return content_;}
private:
	std::string		url_;			//当前帖子地址
	std::string		title_;			//帖子标题
	std::string		user_id_;		//发帖楼主id
	std::string		username_;		//发帖楼主名
	int64			post_time_;		//发帖unix时间
//	std::string		content_;		//回帖内容
	std::string		addr_;
};
#endif

class TiebaTask: public RobotTask {
public:
	TiebaTask() {type_ = TIEBA;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual void GetDataFromDb(base_logic::DictionaryValue* dict);
	virtual RobotTaskBase* CreateTaskPacketUnit();
	virtual void SetTaskPacketUnit(RobotTaskBase *task);
	virtual std::string SerializeSelf();

public:
//	void set_addr(const std::string &addr) {addr_ = addr;}
//	std::string addr() const {return addr_;}

	void set_floor_num(int32 floor_num) {floor_num_ = floor_num;}
	uint64 floor_num() const {return floor_num_;}

//	void set_url(const std::string &url) {url_ = url;}
//	std::string url() const {return url_;}

	void set_kw(const std::string &kw) {kw_ = kw;}
	std::string kw() const {return kw_;}

	void set_fid(const std::string &fid) {fid_ = fid;}
	std::string fid() const {return fid_;}

	void set_tbs(const std::string &tbs) {tbs_ = tbs;}
	std::string tbs() const {return tbs_;}

//	void set_content(const std::string &content) {content_ = content;}
//	std::string content() const {return content_;}

	void set_repost_id(const std::string &repost_id) {repost_id_ = repost_id;}
	std::string repost_id() const {return repost_id_;}
private:
//	std::string		url_;			//当前帖子地址
	std::string		kw_;			//贴吧名
	std::string		fid_;
	std::string		tbs_;
	int32			floor_num_;		//楼层id
	std::string		repost_id_;		//回得楼层编号
};

#if 0
class QZoneTask: public RobotTask {
public:
	QZoneTask(): host_uin_(0) {type_ = QZONE;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
public:
	void set_addr(const std::string &addr) {addr_ = addr;}
	std::string addr() const {return addr_;}
	void set_topic_id(const std::string &topic_id) {topic_id_ = topic_id;}
	std::string topic_id() const {return topic_id_;}
	void set_host_uin(int64 host_uin) {host_uin_ = host_uin;}
	int64 host_uin() const {return host_uin_;}
//	void set_content(const std::string &content) {content_ = content;}
//	std::string content() const {return content_;}
private:
	std::string		topic_id_;		//留言对象
	int64			host_uin_;		//对方的 qq 号
	std::string		content_;		//回复内容
	std::string		addr_;
};
#endif

class Taoguba: public RobotTask {
public:
	Taoguba() {type_ = TAOGUBA;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual void GetDataFromDb(base_logic::DictionaryValue* dict);
	virtual RobotTaskBase* CreateTaskPacketUnit();
	virtual void SetTaskPacketUnit(RobotTaskBase *task);
	virtual std::string SerializeSelf();

	std::string topic_id() const {return topic_id_;}
	std::string subject() const {return subject_;}
private:
	std::string topic_id_;
	std::string subject_;
};

class RobotTaskFactory {
public:
	static RobotTask* Create(RobotTask::TaskType type);
};

}  // namespace base_logic


#endif /* CRAWLER_MANGER_INFOS_H_ */
