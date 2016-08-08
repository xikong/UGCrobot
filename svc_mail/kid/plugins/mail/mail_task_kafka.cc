/*
 * mail_task_kafka.cc
 *
 *  Created on: 2016年6月13日
 *      Author: Harvey
 */

#include "mail_task_kafka.h"

namespace mail_logic {

MailTaskConsumer::MailTaskConsumer() {
    if (!Initialize()) {
        assert(0);
    }
}

MailTaskConsumer::~MailTaskConsumer() {
    kafka_consumer_.Close();
}

bool MailTaskConsumer::Initialize() {
    int ret = kafka_consumer_.Init(0, "mail_robot_topic", "222.73.57.12:9092", NULL);
    if (CONSUMER_INIT_SUCCESS != ret) {
        LOG_ERROR("MailTask Kafka Init Fail");
        return false;
    }

    LOG_DEBUG("MailTask Kafka Init Success");
    return true;
}

void MailTaskConsumer::FectchTasks(std::list<string> *list) {

    std::set<std::string> data_list;
    std::string data;

    // todo 换成性能更高的回调函数模式，需爬虫流量控制功能配合
    for (int i = 0; i < 80; i++) {

        int pull_re = kafka_consumer_.PullData(data);
        if (CONSUMER_CONFIG_ERROR == pull_re) {
            LOG_MSG2("CONSUMER_CONFIG_ERROR,pull_re=%d", pull_re);
        }

        if (PULL_DATA_TIMEOUT == pull_re) {
            LOG_MSG2("consumer get url timeout,pull_re=%d", pull_re);
            break;
        }
        data_list.insert(data);
    }

    bool r = false;
    for (std::set<std::string>::iterator it = data_list.begin();
            it != data_list.end(); it++) {
        std::string data = *it;
        base_logic::ValueSerializer* engine =
                base_logic::ValueSerializer::Create(0, &data);
        int error_code = 0;
        std::string error_str;
        base_logic::Value* value = engine->Deserialize(&error_code, &error_str);
        if (0 != error_code || NULL == value){
            continue;
        }

        base_logic::DictionaryValue* task_info_dic =
                (base_logic::DictionaryValue*) value;

        string mail_receiver = "";
        r = task_info_dic->GetString(L"mail_to", &mail_receiver);
        if( r && !mail_receiver.empty() ){
            list->push_back(mail_receiver);
        }

        delete task_info_dic;
        base_logic::ValueSerializer::DeleteSerializer(0, engine);
    }

    LOG_DEBUG2("fetch kafka mailtask complete, receive task num:%d", list->size());
}

} /* namespace mail_logic */
