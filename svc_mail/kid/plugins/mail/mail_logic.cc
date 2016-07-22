/*
 * mail_logic.cpp
 *
 *  Created on: 2016年6月13日
 *      Author: Harvey
 */

#include "mail_logic.h"
#include "mail/mail_schedule.h"

#include <fstream>

namespace mail_logic {

const string MailLogic::g_default_config_path_ = "./plugins/mail/mail_config.json";

const int MailLogic::TIME_FETCH_KAFKA_TASK = 10001;
const int MailLogic::TIME_FETCH_DB_CONTENT = 10002;
const int MailLogic::TIME_EXCUTE_MAIL_TASK = 10003;

MailLogic::MailLogic(){
    if( !Initialize() || !BeFirstStart() ){
        LOG_ERROR("MailLogic Initialize Failed");
        exit(0);
    }
}

MailLogic * MailLogic::GetPtr(){
    return &GetRef();
}

MailLogic & MailLogic::GetRef(){
    static MailLogic instance;
    return instance;
}

bool MailLogic::Initialize(){

    manager_db_.reset( new MailDB() );

    Json::Reader reader;
    Json::Value root;
    std::ifstream is;
    is.open(g_default_config_path_.c_str(), std::ios::binary);
    if( !reader.parse(is, root) ){
        LOG_ERROR("Json::Reader Parse Config Failed");
        return false;
    }

    int mysql_size = root["mysql"].size();
    if( mysql_size <= 0 ){
        return false;
    }

    for(int i = 0; i < mysql_size; ++i){
        Json::Value mysql_val = root["mysql"][i];

        string host = mysql_val["host"].asString();
        int port = mysql_val["port"].asInt();
        string user = mysql_val["user"].asString();
        string pass = mysql_val["pass"].asString();
        string name = mysql_val["name"].asString();
        base::ConnAddr mysql_addr(host, port, user, pass, name);

        //Init Mysql
        config::FileConfig config;
        config.mysql_db_list_.push_back(mysql_addr);
        base_logic::DataControllerEngine::Init(&config);
    }

    int acc_size = root["MailSendAccounts"].size();
    for( int i = 0; i < acc_size; ++i ){
        Json::Value acc_val = root["MailSendAccounts"][i];
        struct MailAccountInfo *mail_acc = new struct MailAccountInfo;
        mail_acc->username_ = acc_val["account"].asString();
        mail_acc->password_ = acc_val["password"].asString();
        mail_acc->hostname_ = acc_val["smtp_domain"].asString();
        MailSchedule::GetRef().SaveSenderMailInfo(mail_acc);
        LOG_DEBUG2("LoadConfig MailAccout, acc = %s, hostname = %s",
                   mail_acc->username_.c_str(), mail_acc->hostname_.c_str());
    }

    is.close();
    return true;
}

bool MailLogic::BeFirstStart(){
    //load all mail_content
    if( !OnTimeFetchDBContent(true) ){
        LOG_ERROR(" not have enough mail content ");
        return false;
    }
    return true;
}

bool MailLogic::CheckMailAddrValid(const string &email){
    if( string::npos == email.find("@")){
        return false;
    }

    return true;
}

bool MailLogic::CheckMailTargetIsFirstDB(const string &mail_to){
    bool is_exist = false;
    manager_db_->CheckMailTargetIsFirst(mail_to, is_exist);
    return is_exist;
}

bool MailLogic::SaveComleteMailTask(const MailTask &mail_task){
    return manager_db_->SaveComleteMailTask(mail_task);
}

bool MailLogic::OnMailMessage(struct server *srv, const int socket,
                              const void *msg, const int len){
    LOG_DEBUG2("Receive Msg = %s", msg);
    return true;
}

bool MailLogic::OnIniTimer(struct server *srv){
    if( NULL != srv->add_time_task){
        srv->add_time_task(srv, "mail", TIME_FETCH_KAFKA_TASK, 5, -1);
        srv->add_time_task(srv, "mail", TIME_FETCH_DB_CONTENT, 10, -1);
        srv->add_time_task(srv, "mail", TIME_EXCUTE_MAIL_TASK, 30, -1);
    }
    return true;
}

bool MailLogic::OnTimeout(struct server *srv, char* id, int opcode, int time){
    switch(opcode){
        case TIME_FETCH_KAFKA_TASK:{
            OnTimeFetchMailTask();
            break;
        }
        case TIME_FETCH_DB_CONTENT:{
            OnTimeFetchDBContent();
            break;
        }
        case TIME_EXCUTE_MAIL_TASK:{
            OnTimeExcuteMailTask();
            break;
        }
        default:{
            break;
        }
    }
    return true;
}

bool MailLogic::OnTimeFetchMailTask(){
    std::list<string> list;
    mail_task_kafka_.FectchTasks(&list);
    std::list<string>::iterator iter(list.begin());
    for( ; iter != list.end(); ++iter ){
        if(!CheckMailAddrValid((*iter))){
            LOG_DEBUG2("MailLogic::OnTimeFetchMailTask Task MailAddr valid %s",
                       (*iter).c_str());
            continue;
        }
        MailSchedule::GetRef().PushNewMailTask(*iter);
    }
    return true;
}

bool MailLogic::OnTimeFetchDBContent(bool is_first){
    std::list<base_logic::MailContentInfo *> list;
    manager_db_->FetchBatchTaskContent(&list, is_first);
    if( list.empty() ){
        return false;
    }

    std::list<base_logic::MailContentInfo *>::iterator iter(list.begin());
    for( ; iter != list.end(); ++iter ){
        MailSchedule::GetRef().SaveNewMailContent(*iter);
    }
    return true;
}

void MailLogic::OnTimeExcuteMailTask(){
    MailSchedule::GetRef().OnTimerExcuteTask();
}

} /* namespace mail_logic */
