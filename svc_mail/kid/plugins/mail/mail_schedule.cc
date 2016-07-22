/*
 * mail_schedule.cc
 *
 *  Created on: 2016年6月14日
 *      Author: Harvey
 */

#include "mail/mail_schedule.h"

namespace mail_logic {

const int MailSchedule::max_recevier_ = 10;
MailSchedule::MailSchedule()
        : curr_sender_(0) {
    if (Initialize()) {
        assert(0);
    }
}

MailSchedule::~MailSchedule() {
    DeinitThreadrw(lock_);
}

bool MailSchedule::Initialize(){
    InitThreadrw(&lock_);
}

MailSchedule * MailSchedule::GetPtr(){
    return &GetRef();
}

MailSchedule & MailSchedule::GetRef(){
    static MailSchedule schedule;
    return schedule;
}

bool MailSchedule::SaveSenderMailInfo(MailAccountInfo *mail_acc){
    base_logic::WLockGd lk(lock_);
    all_account_vec_.push_back(mail_acc);
    return true;
}

bool MailSchedule::PostThreadTaskReady(){
    struct server *srv = logic::CoreSoUtils::GetSRV();
    if(NULL == srv){
        LOG_ERROR("GetSRV() NULL");
        return false;
    }

    if(NULL == srv->user_addtask){
        LOG_ERROR("user_addtask NULL");
        return false;
    }

    int ret = srv->user_addtask(srv, MAIL_TASK_READY, &mail_pl_);
    if( 0 != ret ){
        LOG_ERROR("user_addtask fail");
        return false;
    }
    return true;
}

void MailSchedule::OnThreadFun(struct server *srv, int fd, void* data){
    switch(fd){
        case MAIL_TASK_READY:{
            StartMailSendTask();
            break;
        }
        default:{
            break;
        }
    }
}

void MailSchedule::PushNewMailTask(const string &mail_receiver){
    bool r = false;
    base_logic::WLockGd lk(lock_);
    string current_time = logic::SomeUtils::GetLocalTime(time(NULL));

    //check this task is complete
    ALL_RECEIVE_TASK_MAP::iterator iter =
            all_receive_task_map_.find(mail_receiver);
    if( iter != all_receive_task_map_.end() ){
        LOG_DEBUG2("MailSchedule::PushNewMailTask MailExists, target = %s, time = %s",
                   mail_receiver.c_str(), current_time.c_str());
        return;
    }else{
        r = MailLogic::GetRef().CheckMailTargetIsFirstDB(mail_receiver);
        if( r ){
            LOG_DEBUG2("MailSchedule::PushNewMailTask MailExists, target = %s, time = %s",
                       mail_receiver.c_str(), current_time.c_str());
            return;
        }
    }

    //push task in wait excute queue
    mail_task_queue_.push(mail_receiver);
    all_receive_task_map_.insert(std::make_pair(mail_receiver, true));
    LOG_DEBUG2("Receive New Mail Task, Curr Queue Size = %d, CurrentTime = %s",
               mail_task_queue_.size(), current_time.c_str());
}

void MailSchedule::OnTimerExcuteTask(){
    base_logic::RLockGd lk(lock_);
    if(mail_task_queue_.size() > 0){
        PostThreadTaskReady();
    }
}

bool MailSchedule::PopNewMailTask(string &mail_receiver){
    base_logic::WLockGd lk(lock_);
    if( mail_task_queue_.size() <= 0 ){
        return false;
    }

    mail_receiver = mail_task_queue_.front();
    mail_task_queue_.pop();
    LOG_DEBUG2("pop new mail task, wait send queue size = %d", mail_task_queue_.size());
    return true;
}

void MailSchedule::StartMailSendTask(){
    bool r = false;
    string mail_targets;
    for( int i = 0; i < max_recevier_; ++i ){
        string mail_receiver;
        r = PopNewMailTask(mail_receiver);
        if( !r ){
            break;
        }
        mail_targets += mail_receiver + ";";
    }

    mail_targets = mail_targets.substr(0, mail_targets.size() - 1);
    LOG_DEBUG2("mail_targets = %s", mail_targets.c_str());

    base_logic::MailContentInfo *mail_content = NULL;
    r = RondomMailContent(&mail_content);
    if( !r || NULL == mail_content){
        LOG_ERROR("MailSchedule::StartMailSendTask Failed, No Have MailContent");
        return;
    }

    MailAccountInfo *mail_acc = NULL;
    r = GetCurrSenderMailAcc(&mail_acc);
    if( !r || NULL == mail_acc ){
        LOG_ERROR("MailSchedule::StartMailSendTask Failed, No Have MailAccount");
        return;
    }

    /*
     * python SendmailViaSMTP.py
     * --host="mail.domain.com"
     * --from="myname@yourdomain.com"
     * --to="friends1@domain1.com;friends2@domain.com"
     * --user="myname@yourdomain.com"
     * --password="p4word"
     * --subject="mail title"
     * --file="/path/to/file"
     *
     * */

    std::stringstream os;
    os << "python ./python/SendmailViaSMTP.py";
    os << " --host=" << mail_acc->hostname_;
    os << " --from=" << mail_acc->username_;
    os << " --to=" << mail_targets;
    os << " --user=" << mail_acc->username_;
    os << " --password=" << mail_acc->password_;
    os << " --subject=" << mail_content->mailsubject_;
    os << " --content=" << mail_content->mailbody_;
    string commond = os.str();

    FILE *fp = popen(commond.c_str(), "r");
    if( NULL == fp ){
        LOG_ERROR2("popen mail_woker eror, err = %s", strerror(errno));
        return;
    }

    char buf[MAXLINE];
    while( fgets(buf, MAXLINE, fp) != NULL ){
        LOG_DEBUG2("%s", buf);
    }

    // close popen file
    pclose(fp);

    // record mail log db
    MailTask task_log;
    task_log.to_ = mail_targets;
    task_log.mail_content_id_ = mail_content->id_;
    r = MailLogic::GetRef().SaveComleteMailTask(task_log);
    if( !r ){
        LOG_ERROR("MailSchedule::StartMailSendTask SaveCompleteMailTask Failed");
    }
}

void MailSchedule::SaveNewMailContent(base_logic::MailContentInfo *mail_content){
    base_logic::WLockGd lk(lock_);
    mail_conntent_vec_.push_back(mail_content);
}

bool MailSchedule::RondomMailContent(base_logic::MailContentInfo **mail_content){
    base_logic::WLockGd lk(lock_);
    srand(time(NULL));
    int pos = rand() % mail_conntent_vec_.size();
    if( pos < 0 || pos >= mail_conntent_vec_.size() ){
        return false;
    }

    *mail_content = mail_conntent_vec_.at(pos);
    return true;
}

bool MailSchedule::GetCurrSenderMailAcc(MailAccountInfo **mail_acc){
    base_logic::WLockGd lk(lock_);
    if( all_account_vec_.empty() ){
        return false;
    }

    curr_sender_++;
    curr_sender_ = curr_sender_ % (int)all_account_vec_.size();
    *mail_acc = all_account_vec_.at(curr_sender_);
    return true;
}

} /* namespace mail_logic */
