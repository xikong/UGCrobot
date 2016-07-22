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
    TASK_EXECUED = 7,
	TASK_FAIL = 8,
	TASK_SUCCESS = 9
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

class TaskInfo {
 public:
    TaskInfo();

    TaskInfo(const TaskInfo& task);

    TaskInfo& operator = (const TaskInfo& task);

    ~TaskInfo() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int64 id) {data_->id_ = id;}
    void set_depth(const int8 depth) {data_->depth_ = depth;}
    void set_machine(const int8 machine) {data_->machine_ = machine;}
    void set_storage(const int8 storage) {data_->storage_ = storage;}
    void set_method(const int8 method) {data_->method_ = method;}
    void set_is_login(const int8 is_login) {data_->is_login_ = is_login;}
    void set_is_finish(const int8 is_finish) {data_->is_finish_ = is_finish;}
    void set_is_forge(const int8 is_forge) {data_->is_forge_ = is_forge;}
    void set_is_over(const int8 is_over) {data_->is_over_ = is_over;}
    void set_state(const int8 state) {data_->state_ = state;}
    void set_url(const std::string& url) {data_->url_ = url;}
    void set_crawl_num(const int64 crawl_num) {data_->crawl_num_ = crawl_num;}
    void set_attrid(const int64 attrid) {data_->attrid_ = attrid;}
    void set_polling_time(const int64 polling_time) {
        data_->polling_time_ = polling_time;
    }

    void set_base_polling_time(const int64 base_polling_time) {
        data_->base_polling_time_ = base_polling_time;
    }

    void set_last_task_time(const int64 last_task_time) {
        data_->last_task_time_ = last_task_time;
    }

    void set_current_depth(const int8 current_depth) {
        data_->cur_depth_ = current_depth;
    }

    void set_type(const int8 type) {
        data_->type_ = type;
    }

    void update_time(const int64 update_time = 0,
            const int64 radom_num = 0,
            bool is_first = false) {
        if (update_time == 0)
            data_->last_task_time_ = time(NULL);
        else
            data_->last_task_time_ = update_time;

        //  更新下一轮时间 避免多个任务同一时间执行
        if (is_first)
            data_->polling_time_ =
                    (time(NULL) + radom_num) % data_->base_polling_time_;
        else
            data_->polling_time_ =
                (time(NULL) + radom_num) % data_->base_polling_time_
                + data_->base_polling_time_;
    }

    const int64 id() const {return data_->id_;}
    const int64 crawl_num() const {return data_->crawl_num_;}
    const int8 depth() const {return data_->depth_;}
    const int8 cur_depth() const {return data_->cur_depth_;}
    const int8 machine() const {return data_->machine_;}
    const int8 storage() const {return data_->storage_;}
    const int8 method() const {return data_->method_;}
    const int8 state() const {return data_->state_;}
    const int8 is_login() const {return data_->is_login_;}
    const int8 is_finish() const {return data_->is_finish_;}
    const int8 is_forge() const {return data_->is_forge_;}
    const int8 is_over() const {return data_->is_over_;}
    const int8 type() const {return data_->type_;}
    const int64 create_time() const {return data_->create_time_;}
    const int64 polling_time() const {return data_->polling_time_;}
    const int64 base_polling_time() const {return data_->base_polling_time_;}
    const int64 last_task_time() const {return data_->last_task_time_;}
    const int64 attrid() const {return data_->attrid_;}
    const std::string url() const {return data_->url_;}
	const int refcount() const {return data_->refcount();}

    void ValueSerialization(base_logic::DictionaryValue* dict);

 private:
    class Data{
     public:
       Data()
        : refcount_(1)
        , id_(0)
        , type_(UNKNOW_TASK)
        , depth_(1)
        , cur_depth_(1)
        , machine_(0)
        , storage_(0)
        , method_(0)
        , state_(0)
        , is_over_(0)
        , is_login_(0)
        , is_finish_(0)
        , is_forge_(0)
        , crawl_num_(0)
        , attrid_(0)
        , polling_time_(10)
        , base_polling_time_(10)
        , create_time_(time(NULL))
        , last_task_time_(time(NULL)) {
       }

     public:
        int64        id_;
        int8         type_;
        int8         depth_;
        int8         cur_depth_;
        int8         machine_;
        int8         storage_;
        int8         is_login_;
        int8         is_finish_;
        int8         is_forge_;
        int8         is_over_;
        int8         state_;
        int8         method_;
        int64        attrid_;
        int64        polling_time_;
        int64        base_polling_time_;
        int64        last_task_time_;
        int64        create_time_;
        int64        crawl_num_;
        std::string  url_;
        void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

        void Release() {__sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_)delete this;
        }
		int refcount() const {return refcount_;}
     private:
        int      refcount_;
    };

    Data*       data_;
};

class WeiboTaskInfo {
 public:
    WeiboTaskInfo();

    WeiboTaskInfo(const WeiboTaskInfo& task);

    WeiboTaskInfo& operator = (const WeiboTaskInfo& task);

    ~WeiboTaskInfo() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int64 id) {data_->id_ = id;}
    void set_depth(const int8 depth) {data_->depth_ = depth;}
    void set_machine(const int8 machine) {data_->machine_ = machine;}
    void set_storage(const int8 storage) {data_->storage_ = storage;}
    void set_method(const int8 method) {data_->method_ = method;}
    void set_is_login(const int8 is_login) {data_->is_login_ = is_login;}
    void set_is_finish(const int8 is_finish) {data_->is_finish_ = is_finish;}
    void set_is_forge(const int8 is_forge) {data_->is_forge_ = is_forge;}
    void set_is_over(const int8 is_over) {data_->is_over_ = is_over;}
    void set_state(const int8 state) {data_->state_ = state;}
    void set_url(const std::string& url) {data_->url_ = url;}
    void set_crawl_num(const int64 crawl_num) {data_->crawl_num_ = crawl_num;}
    void set_attrid(const int64 attrid) {data_->attrid_ = attrid;}
    void set_polling_time(const int64 polling_time) {
        data_->polling_time_ = polling_time;
    }

	void set_cookie_id(const int64 cookie_id) {data_->cookie_id_ = cookie_id;}
	const int64 cookie_id() const {return data_->cookie_id_;}
	void set_cookie(const std::string &cookie) {data_->cookie_ = cookie;}
	const std::string cookie() const {return data_->cookie_;}
	void set_content(const std::string &content) {data_->content_ = content;}
	const std::string content() const {return data_->content_;}
	void set_addr(const std::string &addr) {data_->addr_ = addr;}
	const std::string addr() const {return data_->addr_;}
	void set_topic_id(const std::string &topic_id) {data_->topic_id_ = topic_id;}
	const std::string topic_id() const {return data_->topic_id_;}
	void set_host_uin(const std::string &host_uin) {data_->host_uin_ = host_uin;}
	const std::string host_uin() const {return data_->host_uin_;}

    void set_base_polling_time(const int64 base_polling_time) {
        data_->base_polling_time_ = base_polling_time;
    }

    void set_last_task_time(const int64 last_task_time) {
        data_->last_task_time_ = last_task_time;
    }

    void set_current_depth(const int8 current_depth) {
        data_->cur_depth_ = current_depth;
    }

    void set_type(const int8 type) {
        data_->type_ = type;
    }

    void update_time(const int64 update_time = 0,
            const int64 radom_num = 0,
            bool is_first = false) {
        if (update_time == 0)
            data_->last_task_time_ = time(NULL);
        else
            data_->last_task_time_ = update_time;

        //  更新下一轮时间 避免多个任务同一时间执行
        if (is_first)
            data_->polling_time_ =
                    (time(NULL) + radom_num) % data_->base_polling_time_;
        else
            data_->polling_time_ =
                (time(NULL) + radom_num) % data_->base_polling_time_
                + data_->base_polling_time_;
    }

    const int64 id() const {return data_->id_;}
    const int64 crawl_num() const {return data_->crawl_num_;}
    const int8 depth() const {return data_->depth_;}
    const int8 cur_depth() const {return data_->cur_depth_;}
    const int8 machine() const {return data_->machine_;}
    const int8 storage() const {return data_->storage_;}
    const int8 method() const {return data_->method_;}
    const int8 state() const {return data_->state_;}
    const int8 is_login() const {return data_->is_login_;}
    const int8 is_finish() const {return data_->is_finish_;}
    const int8 is_forge() const {return data_->is_forge_;}
    const int8 is_over() const {return data_->is_over_;}
    const int8 type() const {return data_->type_;}
    const int64 create_time() const {return data_->create_time_;}
    const int64 polling_time() const {return data_->polling_time_;}
    const int64 base_polling_time() const {return data_->base_polling_time_;}
    const int64 last_task_time() const {return data_->last_task_time_;}
    const int64 attrid() const {return data_->attrid_;}
    const std::string url() const {return data_->url_;}
	const int refcount() const {return data_->refcount();}

    void ValueSerialization(base_logic::DictionaryValue* dict);

 private:
    class Data{
     public:
       Data()
        : refcount_(1)
        , id_(0)
        , type_(UNKNOW_TASK)
        , depth_(1)
        , cur_depth_(1)
        , machine_(0)
        , storage_(0)
        , method_(0)
        , state_(0)
        , is_over_(0)
        , is_login_(0)
        , is_finish_(0)
        , is_forge_(0)
        , crawl_num_(0)
        , attrid_(5)
        , polling_time_(10)
        , base_polling_time_(10)
        , create_time_(time(NULL))
        , last_task_time_(time(NULL)) {
       }

     public:
        int64        id_;
        int8         type_;
        int8         depth_;
        int8         cur_depth_;
        int8         machine_;
        int8         storage_;
        int8         is_login_;
        int8         is_finish_;
        int8         is_forge_;
        int8         is_over_;
        int8         state_;
        int8         method_;
        int64        attrid_;
        int64        polling_time_;
        int64        base_polling_time_;
        int64        last_task_time_;
        int64        create_time_;
        int64        crawl_num_;
        std::string  url_;

		int64		 cookie_id_;
		std::string  cookie_;
		std::string	 content_;
		std::string	 addr_;
		std::string	 topic_id_;
		std::string	 host_uin_;
        void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

        void Release() {__sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_)delete this;
        }
		int refcount() const {return refcount_;}
     private:
        int      refcount_;
    };

    Data*       data_;
};

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

    static bool cmp(const ForgeryIP& t_info, const ForgeryIP& r_info);

    void ValueSerialization(base_logic::DictionaryValue* dict);


    class Data {
     public:
        Data()
         : refcount_(1)
         , id_(0)
         , type_(0)
         , count_(0)
     	 , send_last_time_(0) {}

     public:
        int32       id_;
        int8        type_;
        int64       count_;
        time_t		send_last_time_;
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
    void ValueSerialization(base_logic::DictionaryValue* dict);

    class Data{
     public:
        Data()
         : refcount_(1)
         , id_(0)
         , type_(0)
         , count_(0) {}

     public:
        int32       id_;
        int8        type_;
        int64       count_;
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

class TaskPlatDescription {
 public:
    TaskPlatDescription();
    TaskPlatDescription(const TaskPlatDescription& task_info);

    TaskPlatDescription& operator = (const TaskPlatDescription& task_info);
    ~TaskPlatDescription() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int64 id) {data_->id_ = id;}
    void set_depth(const int8 depth) {data_->depth_ = depth;}
    void set_machine(const int8 machine) {data_->machine_ = machine;}
    void set_storage(const int8 storage) {data_->storage_ = storage;}
    void set_over(const int8 over) {data_->over_ = over;}
    void set_forge(const int8 forge) {data_->forge_ = forge;}
    void set_description(const std::string& description) {
        data_->description_ = description;
    }

    const int64 id() const {return data_->id_;}
    const int8 depth() const {return data_->depth_;}
    const int8 machine() const {return data_->machine_;}
    const int8 storage() const {return data_->storage_;}
    const int8 over() const {return data_->over_;}
    const int8 forge() const {return data_->forge_;}
    const std::string& description() const {return data_->description_;}

    void ValueSerialization(base_logic::DictionaryValue* dict);


    class Data {
     public:
        Data()
         : refcount_(1)
         , id_(0)
         , depth_(0)
         , machine_(0)
         , storage_(0)
         , over_(0)
         , forge_(0) {}

     public:
        int64       id_;
        int8        depth_;
        int8        machine_;
        int8        storage_;
        int8        over_;
        int8        forge_;
        std::string description_;

        void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

        void Release() {__sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_)delete this;
        }

     private:
        int refcount_;
    };

    Data*     data_;
};

class StorageInfo {
  public:
    StorageInfo();
    StorageInfo(const StorageInfo& info);

    StorageInfo& operator = (const StorageInfo& info);

    ~StorageInfo() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int64 id) {data_->id_ = id;}
    void set_taskid(const int64 task_id) {data_->task_id_ = task_id;}
    void set_type(const int8 type) {data_->type_ = type;}
    void set_max_depth(const int8 max_depth) {data_->max_depth_ = max_depth;}
    void set_cur_depth(const int8 cur_depth) {data_->cur_depth_ = cur_depth;}
    void set_name(const std::string& key_name) {data_->key_name_ = key_name;}
    void set_hkey(const std::string& pos_name) {data_->pos_name_ = pos_name;}
    void set_time(const std::string& time) {data_->time_ = time;}
    void set_attrid(const int32 attr_id) {data_->attr_id_ = attr_id;}
    void set_state(const int8 state) {data_->state_ = state;}

    const int64 id() const {return data_->id_;}
    const int64 taskid() const {return data_->task_id_;}
    const int32 attrid() const {return data_->attr_id_;}
    const int8 max_depth() const {return data_->max_depth_;}
    const int8 cur_depth() const {return data_->cur_depth_;}
    const int8 state() const {return data_->state_;}
    const int8 type() const {return data_->type_;}
    const std::string& key_name() const {return data_->key_name_;}
    const std::string& pos_name() const {return data_->pos_name_;}
    const std::string& time() const {return data_->time_;}

    void ValueSerialization(base_logic::DictionaryValue* dict);

    class Data{
     public:
        Data()
        : refcount_(1)
        , id_(0)
        , type_(0)
        , state_(ANALYTICAL_WAIT)
        , max_depth_(1)
        , cur_depth_(1)
        , task_id_(0)
        , attr_id_(0) {}

     public:
        int64            id_;
        int64            task_id_;
        int32            attr_id_;
        int8             max_depth_;
        int8             cur_depth_;
        int8             state_;
        int8             type_;
        std::string      key_name_;
        std::string      pos_name_;
        std::string      time_;

        void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

        void Release() {__sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_) delete this;
        }

     private:
        int  refcount_;
    };
    Data*       data_;
};


class StorageHBase {
 public:
    StorageHBase();
    StorageHBase(const StorageHBase& hbase);

    StorageHBase& operator = (const StorageHBase& hbase);

    ~StorageHBase() {
        if (data_ != NULL) {
            data_->Release();
        }
    }

    void set_id(const int64 id) {data_->id_ = id;}
    void set_taskid(const int64 task_id) {data_->task_id_ = task_id;}
    void set_max_depth(const int8 max_depth) {data_->max_depth_ = max_depth;}
    void set_cur_depth(const int8 cur_depth) {data_->cur_depth_ = cur_depth;}
    void set_name(const std::string& name) {data_->name_ = name;}
    void set_hkey(const std::string& hkey) {data_->hkey_ = hkey;}
    void set_time(const std::string& time) {data_->time_ = time;}
    void set_attrid(const int32 attr_id) {data_->attr_id_ = attr_id;}
    void set_state(const int8 state) {data_->state_ = state;}

    const int64 id() const {return data_->id_;}
    const int64 taskid() const {return data_->task_id_;}
    const int32 attrid() const {return data_->attr_id_;}
    const int8 max_depth() const {return data_->max_depth_;}
    const int8 cur_depth() const {return data_->cur_depth_;}
    const int8 state() const {return data_->state_;}
    const std::string& name() const {return data_->name_;}
    const std::string& hkey() const {return data_->hkey_;}
    const std::string& time() const {return data_->time_;}

    void ValueSerialization(base_logic::DictionaryValue* dict);

    class Data{
     public:
        Data()
        : refcount_(1)
        , id_(0)
        , state_(ANALYTICAL_WAIT)
        , max_depth_(1)
        , cur_depth_(1)
        , task_id_(0)
        , attr_id_(0) {}

     public:
        int64            id_;
        int64            task_id_;
        int32            attr_id_;
        int8             max_depth_;
        int8             cur_depth_;
        int8             state_;
        std::string      name_;
        std::string      hkey_;
        std::string      time_;

        void AddRef() {__sync_fetch_and_add(&refcount_, 1);}

        void Release() {__sync_fetch_and_sub(&refcount_, 1);
            if (!refcount_) delete this;
        }

     private:
        int  refcount_;
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

class RobotTask {
public:
	enum TaskType{
		UNKNOWN = -1,
		TIEBA = 7001,
		WEIBO,
		TIANYA,
		QZONE
	};
	RobotTask():
		id_(0),
		type_(UNKNOWN),
		state_(TASK_WAIT),
		send_time_(0) {}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict) {}
	virtual void ValueSerialization(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
	virtual std::string GetContentFromDb(base_logic::DictionaryValue* dict);

	void set_id(int64 id) {id_ = id;}
	int64 id() const {return id_;}
	TaskType type() const {return type_;}
	void set_state(TASKSTAE state) {state_ = state;}
	TASKSTAE state() const {return state_;}
	void set_send_time(time_t t = time(NULL)) {send_time_ = t;}
	time_t send_time() const {return send_time_;}
	std::string content() const {return content_;}
	void set_content(std::string con) {content_ = con;}
	void set_url(const std::string &url) {url_ = url;}
	std::string url() const {return url_;}
protected:
	int64		id_;		//任务 id
	TaskType	type_;
	TASKSTAE	state_;
	time_t		send_time_;
	std::string	content_;
	std::string url_;
};

class WeiboTask: public RobotTask {
public:
	WeiboTask() {type_ = WEIBO;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
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
//	std::string		content_;		//回复内容
	std::string		addr_;
};


class TianyaTask: public RobotTask {
public:
	TianyaTask() {type_ = TIANYA;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
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
//	std::string		url_;			//当前帖子地址
	std::string		title_;			//帖子标题
	std::string		user_id_;		//发帖楼主id
	std::string		username_;		//发帖楼主名
	int64			post_time_;		//发帖unix时间
//	std::string		content_;		//回帖内容
	std::string		addr_;
};

class TiebaTask: public RobotTask {
public:
	TiebaTask() {type_ = TIEBA;}
	virtual void GetDataFromKafka(base_logic::DictionaryValue* dict);
	virtual std::string SerializeSelf();
public:
	void set_addr(const std::string &addr) {addr_ = addr;}
	std::string addr() const {return addr_;}

	void set_floor_num(int32 floor_num) {floor_num_ = floor_num;}
	uint64 floor_num() const {return floor_num_;}

	void set_url(const std::string &url) {url_ = url;}
	std::string url() const {return url_;}

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
//	std::string		content_;		//回帖内容
	std::string		repost_id_;		//回得楼层编号
	std::string		addr_;
};

}  // namespace base_logic


#endif /* CRAWLER_MANGER_INFOS_H_ */
