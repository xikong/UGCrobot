/*
 * monitor_logic.h
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MONITOR_MONITOR_LOGIC_H_
#define KID_PLUGINS_MONITOR_MONITOR_LOGIC_H_

#include "monitor/monitor_header.h"
#include "monitor/monitor_db.h"
#include "monitor/monitor_kafka.h"

namespace monitor_logic {

class MonitorLogic{
 public:

    typedef std::map<int, KafkaInfo *> ALL_KAFKA_MAP;

    ~MonitorLogic();
    static MonitorLogic * GetPtr();
    static MonitorLogic & GetRef();

    bool OnIniTimer(struct server *srv);
    bool OnTimeout(struct server *srv, char* id, int opcode, int time);

 private:
    MonitorLogic();
    bool Init();
    void OnTimerFetchKafkaTask();
    void OnTimerFetchNewKafkaTopicDB(bool is_first);
    void OnTimerRecordCurrKafkaTopicNum();

 private:
    scoped_ptr<MonitorDB>   manager_db_;
    static const string     g_default_config_path_;
    struct threadrw_t *     lock_;
    ALL_KAFKA_MAP           all_kafka_map_;

 private:
    static const int        FETCH_KAFKA_TASK_INTERVAL;
    static const int        FETCH_NEW_KAFKA_TOPIC_INTERVAL;
    static const int        RECORD_KAFKA_TOPIC_INTERVAL;

};

} /* namespace monitor_logic */

#endif /* KID_PLUGINS_MONITOR_MONITOR_LOGIC_H_ */
