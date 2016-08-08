/*
 * mail_init.cpp
 *
 *  Created on: 2016年6月13日
 *      Author: Harvey
 */

#include "mail_init.h"

#include "core/common.h"
#include "core/plugins.h"

#include "mail/mail_logic.h";
#include "mail/mail_schedule.h"

typedef struct mail_plugin{
    char* id;
    char* name;
    char* version;
    char* provider;
} MailPlugin;

static void *OnMailStart() {
    signal(SIGPIPE, SIG_IGN);
    MailPlugin *mail = (MailPlugin *)calloc(1, sizeof(MailPlugin));

    mail->id = "mail";
    mail->name = "mail";
    mail->version = "1.0.0";
    mail->provider = "Harvey";

    if( !mail_logic::MailLogic::GetPtr() || !mail_logic::MailSchedule::GetPtr()){
        LOG_ERROR("MailLogic MailScheduler Init Failed");
        assert(0);
    }
    return mail;
}

static handler_t OnMailShutdown(struct server* srv, void* pd) {

    return HANDLER_GO_ON;
}

static handler_t OnMailConnect(struct server *srv, int fd, void *data,
                                int len) {
    return HANDLER_GO_ON;
}

static handler_t OnMailMessage(struct server *srv, int fd, void *data,
                                int len) {
    mail_logic::MailLogic::GetRef().OnMailMessage(srv, fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnMailClose(struct server *srv, int fd) {

    return HANDLER_GO_ON;
}

static handler_t OnUnknow(struct server *srv, int fd, void *data, int len) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastConnect(struct server* srv, int fd, void *data,
                                    int len) {
    return HANDLER_GO_ON;
}

static handler_t OnThreadFunc(struct server *srv, int fd, void* data) {
    mail_logic::MailSchedule::GetRef().OnThreadFun(srv, fd, data);
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastClose(struct server* srv, int fd) {
    return HANDLER_GO_ON;
}

static handler_t OnBroadcastMessage(struct server* srv, int fd, void *data,
                                    int len) {
    mail_logic::MailLogic::GetRef().OnMailMessage(srv, fd, data, len);
    return HANDLER_GO_ON;
}

static handler_t OnIniTimer(struct server* srv) {
    mail_logic::MailLogic::GetRef().OnIniTimer(srv);
    return HANDLER_GO_ON;
}

static handler_t OnTimeOut(struct server* srv, char* id, int opcode, int time) {
    mail_logic::MailLogic::GetRef().OnTimeout(srv, id, opcode, time);
    return HANDLER_GO_ON;
}

int mail_plugin_init(struct plugin *pl) {
    pl->init = OnMailStart;
    pl->clean_up = OnMailShutdown;
    pl->connection = OnMailConnect;
    pl->connection_close = OnMailClose;
    pl->connection_close_srv = OnBroadcastClose;
    pl->connection_srv = OnBroadcastConnect;
    pl->thread_func = OnThreadFunc;
    pl->handler_init_time = OnIniTimer;
    pl->handler_read = OnMailMessage;
    pl->handler_read_srv = OnBroadcastMessage;
    pl->handler_read_other = OnUnknow;
    pl->time_msg = OnTimeOut;
    pl->data = NULL;

    mail_logic::MailSchedule::GetRef().SaveMailPlugin(*pl);
    return 0;
}
