/*
 * robots_init.cc
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#include "robot_check_init.h"
#include "core/common.h"
#include "core/plugins.h"
#include "robot_check/robot_check_logic.h"
#include "robot_check/robot_check_engine.h"

typedef struct robotcheckplugin{
    char* id;
    char* name;
    char* version;
    char* provider;
}RobotCheckPlugin;

static void *OnRobotCheckStart() {

    signal(SIGPIPE, SIG_IGN);

    RobotCheckPlugin *robot_check = (RobotCheckPlugin *)calloc(1,
            sizeof(RobotCheckPlugin));

    robot_check->id = "robot_check";
    robot_check->name = "robot_check";
    robot_check->version = "1.0.0";
    robot_check->provider = "Harvey";

    if (!robot_check_logic::RobotCheckLogic::GetInstance())
        assert(0);

    if (!robot_check_logic::RobotCheckEngine::GetInstance()){
        assert(0);
    }

    return robot_check;
}

static handler_t OnRobotCheckShutdown(struct server* srv, void* pd) {
    robot_check_logic::RobotCheckLogic::FreeInstance();
    robot_check_logic::RobotCheckEngine::FreeInstance();
    return HANDLER_GO_ON;
}

static handler_t OnRobotCheckConnect(struct server *srv, int fd, void *data,
        int len) {
    return HANDLER_GO_ON;
}

static handler_t OnRobotCheckMessage(struct server *srv, int fd, void *data,
        int len) {

    return HANDLER_GO_ON;
}

static handler_t OnRobotCheckClose(struct server *srv, int fd) {

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
    robot_check_logic::RobotCheckEngine::GetInstance()->OnTaskThreadFunc(
            srv, fd, data);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {

    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
        int len) {

    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
    robot_check_logic::RobotCheckLogic::GetInstance()->OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode, int time) {
    robot_check_logic::RobotCheckLogic::GetInstance()->OnTimeout(srv,
            id, opcode, time);
    return HANDLER_GO_ON;
}

int robot_check_plugin_init(struct plugin *pl) {
    pl->init = OnRobotCheckStart;
    pl->clean_up = OnRobotCheckShutdown;
    pl->connection = OnRobotCheckConnect;
    pl->connection_close = OnRobotCheckClose;
    pl->connection_close_srv = OnBroadcastClose;
    pl->connection_srv = OnBroadcastConnect;
    pl->thread_func = OnThreadFunc;
    pl->handler_init_time = OnIniTimer;
    pl->handler_read = OnRobotCheckMessage;
    pl->handler_read_srv = OnBroadcastMessage;
    pl->handler_read_other = OnUnknow;
    pl->time_msg = OnTimeOut;
    pl->data = NULL;

    robot_check_logic::RobotCheckLogic::GetInstance()->SaveRobotCheckPlugin(pl);

    return 0;
}
