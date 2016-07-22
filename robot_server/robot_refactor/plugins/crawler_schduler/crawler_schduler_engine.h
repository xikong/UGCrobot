//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月13日 Author: kerry

#ifndef CRAWLER_SCHDULER_ENGINE_H_
#define CRAWLER_SCHDULER_ENGINE_H_

#include <map>
#include <list>

#include "logic/auto_crawler_infos.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "config/config.h"


namespace router_schduler {

typedef std::map<int32, base_logic::RouterScheduler>	RouterSchedulerIdMap;
typedef std::map<int, base_logic::RouterScheduler>		RouterSchedulerSocketMap;
typedef std::list<base_logic::RouterScheduler>          RouterSchedulerList;
typedef std::map<uint32, int>				RouterIdToRouterSocket;

class SchdulerEngine {
 public:
    virtual bool SetSchduler(const int32 id, void* schduler) = 0;

    virtual bool SetRouterSchduler(const int32 id,
            base_logic::RouterScheduler* schduler) = 0;

    virtual bool GetRouterSchduler(const int32 id,
            base_logic::RouterScheduler* schduler) = 0;

    virtual bool DelRouterSchduler(const int32 id) = 0;

    virtual bool FindRouterSchduler(const int socket,
        base_logic::RouterScheduler* schduler) = 0;

    virtual bool CloseRouterSchduler(const int socket) = 0;

    virtual bool SetRecvTime(const int socket) = 0;

    virtual bool SetSendTime(const int socket) = 0;

    virtual bool CheckHeartPacket(const int socket) = 0;

    virtual bool SendOptimalRouter(const void* data, const int32 len, int16 crawler_type) = 0;

    virtual bool CheckOptimalRouter(int crawler_type) = 0;

    virtual bool SetSendErrorCount(int socket) = 0;

    virtual bool SetRecvErrorCount(int socket) = 0;

    virtual bool SetRouterIdToRouter(uint32 router_id, int socket) = 0;

    virtual void CheckIsEffective() = 0;
};

class SchdulerEngineImpl : public SchdulerEngine {
 public:
    bool SetSchduler(const int32 id, void* schduler);

    bool SetRouterSchduler(const int32 id,
            base_logic::RouterScheduler* schduler);

    bool GetRouterSchduler(const int32 id,
            base_logic::RouterScheduler* schduler);

    bool DelRouterSchduler(const int32 id);

    bool FindRouterSchduler(const int socket,
        base_logic::RouterScheduler* schduler);

    bool CloseRouterSchduler(const int socket);

    bool SetRecvTime(const int socket);

    bool SetSendTime(const int socket);

    bool CheckHeartPacket(const int socket);

    bool SendOptimalRouter(const void* data, const int32 len, int16 crawler_type);

    bool CheckOptimalRouter(int crawler_type);

    bool SetSendErrorCount(int socket);

    bool SetRecvErrorCount(int socket);

    bool SetRouterIdToRouter(uint32 router_id, int socket);

    void CheckIsEffective();
};

struct RouterSchedulerCache {
  RouterSchedulerSocketMap	router_scheduler_socket_map_;
  RouterSchedulerIdMap		router_scheduler_id_map_;
  RouterSchedulerList		router_scheduler_list_;
};

__attribute__((visibility("default")))
class RouterSchdulerManager {
 public:
    RouterSchdulerManager();
    virtual ~RouterSchdulerManager();

 public:
    bool SetRouterSchduler(const int32 id,
           base_logic::RouterScheduler* schduler);

    bool GetRouterSchduler(const int32 id,
           base_logic::RouterScheduler* schduler);

    bool DelRouterSchduler(const int32 id);

    bool FindRouterSchduler(const int socket,
        base_logic::RouterScheduler* schduler);

    bool CloseRouterSchduler(const int socket);

    bool SetRecvTime(const int socket);

    bool SetSendTime(const int socket);

    bool CheckHeartPacket(const int socket);

    bool SendOptimalRouter(const void* data, const int32 len, int16 crawler_type);

    bool CheckOptimalRouter(int crawler_type);

    bool SetSendErrorCount(int socket);

    bool SetRecvErrorCount(int socket);

    bool SetRouterIdToRouter(uint32 router_id, int socket);

    void CheckIsEffective();


 private:
    void Init();

 public:
    static const int TIMEOUT_TICK = 30;
    RouterSchedulerCache* GetFindCache() {return this->schduler_cache_;}

 private:
    struct threadrw_t*             lock_;
    RouterSchedulerCache*          schduler_cache_;
};

class RouterSchdulerEngine {
 private:
    static RouterSchdulerManager           *schduler_mgr_;
    static RouterSchdulerEngine            *schduler_engine_;

    RouterSchdulerEngine() {}
    virtual ~RouterSchdulerEngine() {}
 public:
    __attribute__((visibility("default")))
     static RouterSchdulerManager* GetRouterSchdulerManager() {
        if (schduler_mgr_ == NULL)
            schduler_mgr_ = new RouterSchdulerManager();
        return schduler_mgr_;
    }

    static RouterSchdulerEngine* GetRouterSchdulerEngine() {
        if (schduler_engine_ == NULL)
            schduler_engine_ = new RouterSchdulerEngine();

        return schduler_engine_;
    }
};


}  //  namespace router_schduler


#ifdef __cplusplus
extern "C" {
#endif
router_schduler::SchdulerEngine *GetRouterSchdulerEngine(void);
#ifdef __cplusplus
}
#endif

#endif /* CRAWLER_MANAGER_H_ */
