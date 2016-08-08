/*
 * monitor_logic.cc
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#include "monitor_logic.h"

#include <fstream>

#define ONE_DAY_SEC (24 * 60 * 60)

namespace monitor_logic {

const string MonitorLogic::g_default_config_path_ =
        "./plugins/monitor/monitor_config.xml";

const int MonitorLogic::FETCH_KAFKA_TASK_INTERVAL = 10001;
const int MonitorLogic::FETCH_NEW_KAFKA_TOPIC_INTERVAL = 10002;
const int MonitorLogic::RECORD_KAFKA_TOPIC_INTERVAL = 10003;
const int MonitorLogic::CHECK_NEXT_DAY_ARRIVE_INTERVAL = 10004;

MonitorLogic::MonitorLogic() {
    if (!Init()) {
        assert(0);
    }

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    int curr_hour_sec = tm_now->tm_min * 60 + tm_now->tm_sec;
    if (tm_now->tm_hour > 24) {
        next_back_time_ = now + ONE_DAY_SEC - curr_hour_sec;
    } else {
        next_back_time_ = now + (24 - tm_now->tm_hour) * 60 * 60
                - curr_hour_sec;
    }
}

MonitorLogic::~MonitorLogic() {
    DeinitThreadrw(lock_);
}

bool MonitorLogic::Init() {
    manager_db_.reset(new MonitorDB());
    InitThreadrw(&lock_);

    bool bParseResult = false;
    Json::Reader reader;
    Json::Value root;
    std::ifstream is;
    is.open(g_default_config_path_.c_str(), std::ios::binary);
    if (reader.parse(is, root)) {
        int mysql_size = root["mysql"].size();
        for (int i = 0; i < mysql_size; ++i) {
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
            break;
        }
        if (mysql_size == 1) {
            bParseResult = true;
        }

    } else {
        LOG_ERROR("Json::Reader Parse Config Failed");
    }

    //load db kafka topic
    OnTimerFetchNewKafkaTopicDB(true);

    is.close();
    return bParseResult;
}

MonitorLogic *MonitorLogic::GetPtr() {
    return &GetRef();
}

MonitorLogic &MonitorLogic::GetRef() {
    static MonitorLogic instance;
    return instance;
}

bool MonitorLogic::OnIniTimer(struct server *srv ) {
    if (NULL != srv->add_time_task) {
        srv->add_time_task(srv, "monitor", FETCH_KAFKA_TASK_INTERVAL, 10, -1);
        srv->add_time_task(srv, "monitor", FETCH_NEW_KAFKA_TOPIC_INTERVAL, 15,
                           -1);
        srv->add_time_task(srv, "monitor", RECORD_KAFKA_TOPIC_INTERVAL, 10, -1);
        srv->add_time_task(srv, "monitor", CHECK_NEXT_DAY_ARRIVE_INTERVAL, 1,
                           -1);
    }
    return true;
}

bool MonitorLogic::OnTimeout(struct server *srv, char* id, int opcode,
                             int time ) {
    switch (opcode) {
        case FETCH_KAFKA_TASK_INTERVAL: {
            OnTimerFetchKafkaTask();
            break;
        }
        case FETCH_NEW_KAFKA_TOPIC_INTERVAL: {
            OnTimerFetchNewKafkaTopicDB(false);
            break;
        }
        case RECORD_KAFKA_TOPIC_INTERVAL: {
            OnTimerRecordCurrKafkaTopicNum();
            break;
        }
        case CHECK_NEXT_DAY_ARRIVE_INTERVAL: {
            OnTimerBackUpDB();
            break;
        }
        default: {
            break;
        }
    }
    return true;
}

void MonitorLogic::OnTimerFetchKafkaTask() {
    base_logic::WLockGd lk(lock_);
    ALL_KAFKA_MAP::iterator iter(all_kafka_map_.begin());
    for (; iter != all_kafka_map_.end(); ++iter) {
        int receive_num = 0;
        KafkaInfo *kafka_info = iter->second;
        kafka_info->kafka_.FectchTasks(receive_num);
        if (receive_num > 0) {
            LOG_DEBUG2("kafka_id = %d, topic = %s, pre_num = %d",
                    kafka_info->id_, kafka_info->topic_.c_str(), kafka_info->total_num_);
            kafka_info->total_num_ += receive_num;
            kafka_info->is_need_record_ = true;
            LOG_DEBUG2("kafka_id = %d, topic = %s, curr_num = %d",
                    kafka_info->id_, kafka_info->topic_.c_str(), kafka_info->total_num_);
        }
    }
}

void MonitorLogic::OnTimerFetchNewKafkaTopicDB(bool is_first ) {
    bool r = false;
    base_logic::WLockGd lk(lock_);
    std::list<KafkaInfo *> list;
    manager_db_->FetchNewKafkaTopic(&list, is_first);
    std::list<KafkaInfo *>::iterator iter(list.begin());
    for (; iter != list.end(); ++iter) {
        r = (*iter)->kafka_.Initialize((*iter)->topic_, (*iter)->addr_);
        if (!r) {
            LOG_ERROR2("LoadDB Kafka Init Failed, topic = %s, addr = %s",
                    ((*iter)->topic_).c_str(), ((*iter)->addr_).c_str());
            continue;
        }
        all_kafka_map_[(*iter)->id_] = (*iter);
    }
}

void MonitorLogic::OnTimerRecordCurrKafkaTopicNum() {
    base_logic::WLockGd lk(lock_);
    ALL_KAFKA_MAP::iterator iter(all_kafka_map_.begin());
    for (; iter != all_kafka_map_.end(); ++iter) {
        KafkaInfo *kafka_info = iter->second;
        if (kafka_info->is_need_record_) {
            manager_db_->RecordKafkaTaskNum(kafka_info);
            //init kafka record info
            kafka_info->total_num_ = 0;
            kafka_info->is_need_record_ = false;
        }
    }
}

void MonitorLogic::OnTimerBackUpDB() {
    time_t now = time(NULL);
    if (now >= next_back_time_) {
        manager_db_->BackUpKafkaRecord();
        next_back_time_ += ONE_DAY_SEC;
    }
}

} /* namespace monitor_logic */
