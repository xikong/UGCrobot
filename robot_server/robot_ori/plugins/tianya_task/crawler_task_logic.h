//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry

#ifndef KID_CRAWLER_TASK_TASK_LOGIC_
#define KID_CRAWLER_TASK_TASK_LOGIC_
#include "core/common.h"
#include "crawler_schduler/crawler_schduler_engine.h"
#include "basic/basictypes.h"
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "../../pub/share/manager_info.h"
#include "crawler_task_db.h"
#include "crawler_task_kafka.h"
#include "task_schduler_engine.h"
#include "task_time_manager.h"


namespace tianya_task_logic {

class CrawlerTasklogic {
 public:
    CrawlerTasklogic();
    virtual ~CrawlerTasklogic();

 private:
    static CrawlerTasklogic    *instance_;
    static base_logic::RobotTask::TaskType task_type_;

 public:
    static CrawlerTasklogic *GetInstance();
    static void FreeInstance();

 public:
    bool OnTaskConnect(struct server *srv, const int socket);

    bool OnTaskMessage(struct server *srv, const int socket,
            const void *msg, const int len);

    bool OnTaskClose(struct server *srv, const int socket);

    bool OnBroadcastConnect(struct server *srv, const int socket,
            const void *data, const int len);

    bool OnBroadcastMessage(struct server *srv, const int socket,
            const void *msg, const int len);

    bool OnBroadcastClose(struct server *srv, const int socket);

    bool OnIniTimer(struct server *srv);

    bool OnTimeout(struct server *srv, char* id, int opcode, int time);

 private:
    bool Init();

    bool Startup();

    void InitTask(tianya_task_logic::TaskSchdulerManager* schduler_mgr);

    void TimeDistributionTask();

    void TimeFetchTask();

    bool HandleAllConnect(struct server *srv, const int socket);

    bool HandleAllMessage(struct server *srv, const int socket,
            const void *msg, const int len);

    bool HandleAllClose(struct server *srv, const int socket);
 private:
    void ReplyTaskState(struct server* srv, int socket,
            struct PacketHead *packet, const void *msg = NULL,
            int32 len = 0);

    void RelpyCrawlNum(struct server* srv, int socket,
            struct PacketHead *packet, const void *msg = NULL,
            int32 len = 0);

	bool OnRouterStatus(struct server *srv, int socket, 
		struct PacketHead* packet, const void *msg = NULL, int32 len = NULL);

	bool OnRobotTaskStatus(struct server *srv, int socket,
		struct PacketHead* packet, const void *msg = NULL, int32 len = NULL);
 private:
    plugin_share::ManagerInfo				    *manager_info_;
    scoped_ptr<tianya_task_logic::CrawlerTaskDB>             task_db_;
    scoped_ptr<tianya_task_logic::TaskTimeManager>           task_time_mgr_;
    router_schduler::SchdulerEngine*                         router_schduler_engine_;
};

}  // // namespace crawler_task_logic

#endif

