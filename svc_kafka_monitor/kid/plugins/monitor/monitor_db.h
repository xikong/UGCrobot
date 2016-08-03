/*
 * monitor_db.h
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MONITOR_MONITOR_DB_H_
#define KID_PLUGINS_MONITOR_MONITOR_DB_H_

#include "monitor/monitor_header.h"
#include "monitor/monitor_kafka.h"

namespace monitor_logic {

class MonitorDB{
 public:
    MonitorDB();
    bool FetchNewKafkaTopic(std::list<KafkaInfo *> *list, int is_first );
    bool RecordKafkaTaskNum(KafkaInfo *kafka_info );
    bool BackUpKafkaRecord();

 private:
    static void CallbackFetchNewKafkaTopic(void* param,
                                           base_logic::Value* value );

 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};

} /* namespace monitor_logic */

#endif /* KID_PLUGINS_MONITOR_MONITOR_DB_H_ */
