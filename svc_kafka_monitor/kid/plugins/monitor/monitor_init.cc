/*
 * monitor_init.cc
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#include "monitor_init.h"

#include "core/common.h"
#include "core/plugins.h"

#include "monitor/monitor_header.h"
#include "monitor/monitor_logic.h"

typedef struct monitor_plugin{
    char* id;
    char* name;
    char* version;
    char* provider;
} MonitorPlugin;

static void *OnMonitorStart() {
    signal(SIGPIPE, SIG_IGN);
    MonitorPlugin *monitor = (MonitorPlugin *) calloc(1, sizeof(MonitorPlugin));

    monitor->id = "monitor";
    monitor->name = "monitor";
    monitor->version = "1.0.0";
    monitor->provider = "Harvey";

    if (!monitor_logic::MonitorLogic::GetPtr()) {
        assert(0);
    }
    return monitor;
}

static handler_t OnMonitorShutdown(struct server* srv, void* pd ) {

    return HANDLER_GO_ON;
}

static handler_t OnMonitorConnect(struct server *srv, int fd, void *data,
                                  int len ) {
    return HANDLER_GO_ON;
}

static handler_t OnMonitorMessage(struct server *srv, int fd, void *data,
                                  int len ) {
    return HANDLER_GO_ON;
}

static handler_t OnMonitorClose(struct server *srv, int fd ) {

    return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data, int len ) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv, int fd, void *data,
                                    int len ) {
    return HANDLER_GO_ON;
}

static handler_t OnThreadFunc(struct server *srv, int fd, void* data ) {

    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd ) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
                                    int len ) {
    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv ) {
    monitor_logic::MonitorLogic::GetRef().OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode,
                           int time ) {
    monitor_logic::MonitorLogic::GetRef().OnTimeout(srv, id, opcode, time);
    return HANDLER_GO_ON;
}

int monitor_plugin_init(struct plugin *pl ) {
    pl->init = OnMonitorStart;
    pl->clean_up = OnMonitorShutdown;
    pl->connection = OnMonitorConnect;
    pl->connection_close = OnMonitorClose;
    pl->connection_close_srv = OnBroadcastClose;
    pl->connection_srv = OnBroadcastConnect;
    pl->thread_func = OnThreadFunc;
    pl->handler_init_time = OnIniTimer;
    pl->handler_read = OnMonitorMessage;
    pl->handler_read_srv = OnBroadcastMessage;
    pl->handler_read_other = OnUnknow;
    pl->time_msg = OnTimeOut;
    pl->data = NULL;

    return 0;
}
