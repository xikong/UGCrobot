/*
 * robots_logic.h
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#ifndef UGCROBOT_MASTER_PLUGINS_ROBOTS_ROBOTS_LOGIC_H_
#define UGCROBOT_MASTER_PLUGINS_ROBOTS_ROBOTS_LOGIC_H_

#include <map>
#include <utility>
#include <fcntl.h>
#include <string>
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "net/packet_define.h"
#include "core/common.h"
#include "basic/basictypes.h"
#include "json/reader.h"
#include "json/value.h"
#include "json/json_tool.h"

static const int16 robot_type = 50;

#define SLB_REGISTER_INTERVAL_TIMER     10001
#define HEART_BEAT_CHECK_INTERVAL       10002
#define STATE_REPORT_INTERVAL           10003

namespace robot_logic {

class RobotLogic {
 public:
    virtual ~RobotLogic();

 private:
    //私有构造函数，防止外部构造
    RobotLogic();

    bool InitConfig();

    void RequestSLBRegister();

    int ConnectServer(const std::string host, const int16 port);

    bool RobotRequestRegister(const int socket);

    bool RequestLoginRouter();

    bool LoginRouterResult(const int socket, const void *msg, const int32 len);

    bool HeartBeatCheck();

    void RouterDisconnect();

    bool ReportRobotState();

 public:
    static RobotLogic *GetInstance();

    static void FreeInstance();

    struct plugin *GetRobotPlugin();

    int32 GetRobotId();

    void SaveRobotPlugin(struct plugin *pl);

    bool OnRobotMessage(struct server *srv, const int socket,
            const void *msg, const int len);

    bool OnIniTimer(struct server *srv);

    bool OnTimeout(struct server *srv, char* id, int opcode, int time);

    bool OnBroadcastClose(struct server *srv, const int socket);

    bool RobotRegisterResult(const int socket, const void *msg, const int len);

    bool SendMsgToRouter(struct PacketHead *packet);

 private:
    static RobotLogic       *instance_;
    struct plugin           *robot_pl_;
    struct threadrw_t*      lock_;
    SLBAgent                slb_agent_;
    RouterAgent             router_agent_;
    bool                    is_prase_finsh_;
};

} /* namespace robot_logic */

#endif /* UGCROBOT_MASTER_PLUGINS_ROBOTS_ROBOTS_LOGIC_H_ */
