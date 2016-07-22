/*
 * monitor_kafka.cc
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#include "monitor_kafka.h"

namespace monitor_logic {

MonitorKafka::~MonitorKafka() {
    kafka_consumer_.Close();
}

bool MonitorKafka::Initialize(const string &topic, const string &kafka_addr ) {
    int ret = kafka_consumer_.Init(0, topic.c_str(), kafka_addr.c_str(), NULL);
    if (CONSUMER_INIT_SUCCESS != ret) {
        LOG_ERROR("Monitor Kafka Init Fail");
        return false;
    }

    LOG_DEBUG("Monitor Kafka Init Success");
    return true;
}

void MonitorKafka::FectchTasks(int &count ) {
    count = 0;
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
        ++count;
    } LOG_DEBUG2("fetch kafka task complete, receive task num:%d", count);
}

void KafkaInfo::ValueSerialize(base_logic::DictionaryValue *dict ) {
    dict->GetBigInteger(L"id", &id_);
    dict->GetString(L"topic", &topic_);
    dict->GetString(L"addr", &addr_);
    total_num_ = 0;
    is_need_record_ = false;
    LOG_DEBUG2("LoadDb KafkaInfo Success topic = %s, addr = %s", topic_.c_str(), addr_.c_str());
}

} /* namespace monitor_logic */
