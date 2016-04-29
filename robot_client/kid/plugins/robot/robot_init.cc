/*
 * robots_init.cc
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#include "robot_init.h"
#include "core/common.h"
#include "core/plugins.h"
#include "robot/robot_logic.h"
#include "robot/robot_engine.h"

typedef struct robotplugin{
    char* id;
    char* name;
    char* version;
    char* provider;
}RobotPlugin;

static void *OnRobotStart() {

    signal(SIGPIPE, SIG_IGN);

    RobotPlugin *robot = (RobotPlugin *)calloc(1,
            sizeof(RobotPlugin));

    robot->id = "robot";
    robot->name = "robot";
    robot->version = "1.0.0";
    robot->provider = "Harvey";

    if (!robot_logic::RobotLogic::GetInstance())
        assert(0);

    return robot;
}

static handler_t OnRobotShutdown(struct server* srv, void* pd) {
    robot_logic::RobotLogic::FreeInstance();
    return HANDLER_GO_ON;
}

static handler_t OnRobotConnect(struct server *srv, int fd, void *data,
        int len) {
    return HANDLER_GO_ON;
}

static handler_t OnRobotMessage(struct server *srv, int fd, void *data,
        int len) {

    LOG_MSG2("Receive Client Msg, socket = %d, len = %d", fd, len);
//  robot_logic::RobotLogic::GetInstance()->OnRobotMessage(srv,
//            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnRobotClose(struct server *srv, int fd) {
    robot_logic::RobotLogic::GetInstance()->OnBroadcastClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data,
        int len) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv, int fd,
        void *data, int len) {
    return HANDLER_GO_ON;
}

static handler_t OnThreadFunc(struct server *srv, int fd, void* data){
    robot_logic::RobotEngine::GetInstance()->OnTaskThreadFunc(srv, fd, data);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {
    robot_logic::RobotLogic::GetInstance()->OnBroadcastClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
        int len) {
    robot_logic::RobotLogic::GetInstance()->OnRobotMessage(srv,
            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
    robot_logic::RobotLogic::GetInstance()->OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode, int time) {
    robot_logic::RobotLogic::GetInstance()->OnTimeout(srv,
            id, opcode, time);
    return HANDLER_GO_ON;
}

int robot_plugin_init(struct plugin *pl) {
    pl->init = OnRobotStart;
    pl->clean_up = OnRobotShutdown;
    pl->connection = OnRobotConnect;
    pl->connection_close = OnRobotClose;
    pl->connection_close_srv = OnBroadcastClose;
    pl->connection_srv = OnBroadcastConnect;
    pl->thread_func = OnThreadFunc;
    pl->handler_init_time = OnIniTimer;
    pl->handler_read = OnRobotMessage;
    pl->handler_read_srv = OnBroadcastMessage;
    pl->handler_read_other = OnUnknow;
    pl->time_msg = OnTimeOut;
    pl->data = NULL;

    robot_logic::RobotLogic::GetInstance()->SaveRobotPlugin(pl);

    return 0;
}
