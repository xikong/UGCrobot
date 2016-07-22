//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry
#include "manager/manager_init.h"
#include "core/common.h"
#include "core/plugins.h"
#include "manager/manager_logic.h"


struct managerplugin{
    char* id;
    char* name;
    char* version;
    char* provider;
};


static void *OnManagerStart() {
    signal(SIGPIPE, SIG_IGN);
    struct managerplugin* manager = (struct managerplugin*)calloc(1,
            sizeof(struct managerplugin));
    manager->id = "manager";
    manager->name = "manager";
    manager->version = "1.0.0";
    manager->provider = "kerry";
    if (!manager_logic::Managerlogic::GetInstance())
        assert(0);
    return manager;
}

static handler_t OnManagerShutdown(struct server* srv, void* pd) {
    manager_logic::Managerlogic::FreeInstance();

    return HANDLER_GO_ON;
}

static handler_t OnManagerConnect(struct server *srv, int fd, void *data,
        int len) {
    manager_logic::Managerlogic::GetInstance()->OnManagerConnect(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnManagerMessage(struct server *srv, int fd, void *data,
        int len) {
    manager_logic::Managerlogic::GetInstance()->OnManagerMessage(srv,
            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnManagerClose(struct server *srv, int fd) {
    manager_logic::Managerlogic::GetInstance()->OnManagerClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data,
        int len) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv, int fd,
        void *data, int len) {
    manager_logic::Managerlogic::GetInstance()->OnBroadcastConnect(
            srv, fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {
    manager_logic::Managerlogic::GetInstance()->OnBroadcastClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
        int len) {
    manager_logic::Managerlogic::GetInstance()->OnBroadcastMessage(srv,
            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
    manager_logic::Managerlogic::GetInstance()->OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode, int time) {
    manager_logic::Managerlogic::GetInstance()->OnTimeout(srv,
            id, opcode, time);
    return HANDLER_GO_ON;
}


int manager_plugin_init(struct plugin *pl) {
    pl->init = OnManagerStart;
    pl->clean_up = OnManagerShutdown;
    pl->connection = OnManagerConnect;
    pl->connection_close = OnManagerClose;
    pl->connection_close_srv = OnBroadcastClose;
    pl->connection_srv = OnBroadcastConnect;
    pl->handler_init_time = OnIniTimer;
    pl->handler_read = OnManagerMessage;
    pl->handler_read_srv = OnBroadcastMessage;
    pl->handler_read_other = OnUnknow;
    pl->time_msg = OnTimeOut;
    pl->data = NULL;
    return 0;
}

