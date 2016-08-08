/*
 * mail_logic.h
 *
 *  Created on: 2016年6月13日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MAIL_MAIL_LOGIC_H_
#define KID_PLUGINS_MAIL_MAIL_LOGIC_H_

#include "mail/mail_header.h"

#include "core/common.h"
#include "core/plugins.h"

#include "mail/mail_db.h"
#include "mail/mail_task_kafka.h"

namespace mail_logic {

class MailLogic{
 public:
    ~MailLogic() {}
    static MailLogic * GetPtr();
    static MailLogic & GetRef();

 private:
    MailLogic();
    bool Initialize();
    bool BeFirstStart();
    inline bool CheckMailAddrValid(const string &email);

 public:
    bool CheckMailTargetIsFirstDB(const string &mail_to);
    bool SaveComleteMailTask(const MailTask &mail_task);

 public:
    bool OnMailMessage(struct server *srv, const int socket,
                       const void *msg, const int len);
    bool OnIniTimer(struct server *srv);
    bool OnTimeout(struct server *srv, char* id, int opcode, int time);

 private:
    bool OnTimeFetchMailTask();
    bool OnTimeFetchDBContent(bool is_first = false);
    void OnTimeExcuteMailTask();
    void TestSendMail();

 private:
    int                     curr_send_account_;
    static const string     g_default_config_path_;

    scoped_ptr<MailDB>      manager_db_;
    MailTaskConsumer        mail_task_kafka_;

 private:
    static const int        TIME_FETCH_KAFKA_TASK;
    static const int        TIME_FETCH_DB_CONTENT;
    static const int        TIME_EXCUTE_MAIL_TASK;
};

} /* namespace mail_logic */

#endif /* KID_PLUGINS_MAIL_MAIL_LOGIC_H_ */
