//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry

#include "core/common.h"
#include "core/plugins.h"
#include "crawler_task_init.h"
#include "crawler_task_logic.h"


struct taskplugin{
    char* id;
    char* name;
    char* version;
    char* provider;
};


static void *OnCrawlerTaskStart() {
    signal(SIGPIPE, SIG_IGN);
    struct taskplugin* task = (struct taskplugin*)calloc(
            1, sizeof(struct taskplugin));

    task->id = "tieba_task";

    task->name = "tieba_task";

    task->version = "1.0.0";

    task->provider = "kerry";

    if (!tieba_task_logic::CrawlerTasklogic::GetInstance())
        assert(0);

    return task;
}

static handler_t OnCrawlerTaskShutdown(struct server* srv, void* pd) {
    tieba_task_logic::CrawlerTasklogic::FreeInstance();
    return HANDLER_GO_ON;
}

static handler_t OnCrawlerTaskConnect(struct server *srv, int fd,
        void *data, int len) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnTaskConnect(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnCrawlerTaskMessage(struct server *srv, int fd, void *data,
        int len) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnTaskMessage(srv, fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnCrawlerTaskClose(struct server *srv, int fd) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnTaskClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data,
        int len) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv,
        int fd, void *data, int len) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnBroadcastConnect(srv,
            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnBroadcastClose(srv, fd);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd,
        void *data, int len) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnBroadcastMessage(srv,
            fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id,
        int opcode, int time) {
    tieba_task_logic::CrawlerTasklogic::GetInstance()->OnTimeout(srv, id, opcode, time);
    return HANDLER_GO_ON;
}

int tieba_task_plugin_init(struct plugin *pl) {
    pl->init = OnCrawlerTaskStart;

    pl->clean_up = OnCrawlerTaskShutdown;

    pl->connection = OnCrawlerTaskConnect;

    pl->connection_close = OnCrawlerTaskClose;

    pl->connection_close_srv = OnBroadcastClose;

    pl->connection_srv = OnBroadcastConnect;

    pl->handler_init_time = OnIniTimer;

    pl->handler_read = OnCrawlerTaskMessage;

    pl->handler_read_srv = OnBroadcastMessage;

    pl->handler_read_other = OnUnknow;

    pl->time_msg = OnTimeOut;

    pl->data = NULL;

    return 0;
}

