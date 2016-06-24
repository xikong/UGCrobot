/*
 * monitor_kafka.h
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MONITOR_MONITOR_KAFKA_H_
#define KID_PLUGINS_MONITOR_MONITOR_KAFKA_H_

#include "monitor/monitor_header.h"

#include "logic/base_values.h"
#include "queue/kafka_consumer.h"

namespace monitor_logic {

class MonitorKafka{
 public:
    ~MonitorKafka();
    bool Initialize(const string &topic, const string &kafka_addr);

 public:
    void FectchTasks(int &count);

 private:
    kafka_consumer      kafka_consumer_;
};

class KafkaInfo{
 public:
    void ValueSerialize(base_logic::DictionaryValue *dict);
 public:
    int64           id_;
    std::string     topic_;
    std::string     addr_;
    int64           total_num_;
    MonitorKafka    kafka_;
    bool            is_need_record_;
};

} /* namespace monitor_logic */

#endif /* KID_PLUGINS_MONITOR_MONITOR_KAFKA_H_ */
