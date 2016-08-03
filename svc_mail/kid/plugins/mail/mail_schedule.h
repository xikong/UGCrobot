/*
 * mail_schedule.h
 *
 *  Created on: 2016年6月14日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MAIL_MAIL_SCHEDULE_H_
#define KID_PLUGINS_MAIL_MAIL_SCHEDULE_H_

#include "mail/mail_header.h"

#include "mail/mail_logic.h"

#define MAIL_TASK_READY  10001

namespace mail_logic {

class MailSchedule{
 public:
    typedef std::vector<MailAccountInfo *>              ALL_SENDER_MAIL_VEC;
    typedef std::vector<base_logic::MailContentInfo *>  MAIL_CONTENT_VEC;
    typedef std::queue<string>                          MAIL_TASK_QUEUE;
    typedef std::map<string, bool>                      ALL_RECEIVE_TASK_MAP;

    ~MailSchedule();
    bool Initialize();
    static MailSchedule * GetPtr();
    static MailSchedule & GetRef();

 private:
    MailSchedule();
    bool PostThreadTaskReady();

 public:
    bool SaveSenderMailInfo(MailAccountInfo *mail_acc);
    void OnThreadFun(struct server *srv, int fd, void* data);
    inline void SaveMailPlugin(struct plugin &pl) { mail_pl_ = pl; }
    void PushNewMailTask(const string &mail_receiver);
    void OnTimerExcuteTask();

 private:
    bool PopNewMailTask(string &mail_receiver);
    void StartMailSendTask();

 public:
    void SaveNewMailContent(base_logic::MailContentInfo *mail_content);
    bool RondomMailContent(base_logic::MailContentInfo **mail_content);
    bool GetCurrSenderMailAcc(MailAccountInfo **mail_acc);

 private:
    static const int        max_recevier_;
    struct threadrw_t *     lock_;
    struct plugin           mail_pl_;
    int                     curr_sender_;

    ALL_SENDER_MAIL_VEC     all_account_vec_;
    MAIL_CONTENT_VEC        mail_conntent_vec_;
    MAIL_TASK_QUEUE         mail_task_queue_;
    ALL_RECEIVE_TASK_MAP    all_receive_task_map_;
};

} /* namespace mail_logic */

#endif /* KID_PLUGINS_MAIL_MAIL_SCHEDULE_H_ */
