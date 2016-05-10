//  Copyright (c) 2015-2016 The KID Authors. All rights reserved.
//  Created on: 2016.2.18 Author: yangge

#include "../weibo_task/crawler_task_kafka.h"

#include <mysql.h>
#include <sstream>
#include <set>
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "basic/basic_util.h"
#include "basic/template.h"
#include "basic/radom_in.h"

namespace weibo_task_logic {

CrawlerTaskKafka::CrawlerTaskKafka() {
    if ( CONSUMER_INIT_SUCCESS !=
        kafka_consumer_.Init(0, "newsparser_url", "localhost:9092", NULL))
        LOG_DEBUG("kafka consumer newsparser_url init failed");
    else
        LOG_DEBUG("kafka consumer newsparser_url init success");
}

CrawlerTaskKafka::~CrawlerTaskKafka() {
    kafka_consumer_.Close();
}

bool CrawlerTaskKafka::FectchBatchTempTask(std::list<base_logic::WeiboTask>* list) {
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
    for (std::set<std::string>::iterator it = data_list.begin();
        it != data_list.end(); it++) {
        std::string data = *it;
        base_logic::ValueSerializer* engine =
            base_logic::ValueSerializer::Create(0, &data);
        int error_code = 0;
        std::string error_str;
        base_logic::Value* value = engine->Deserialize(&error_code, &error_str);
        if (0 != error_code || NULL == value)
            continue;
        base_logic::DictionaryValue* task_info_dic =
            (base_logic::DictionaryValue*) value;
        base_logic::WeiboTask task_info;
        task_info.GetDataFromKafka(task_info_dic);
        list->push_back(task_info);
        delete task_info_dic;
        base_logic::ValueSerializer::DeleteSerializer(0, engine);
    }
    LOG_DEBUG2("update task info, total task num:%d", list->size());
    return true;
}

}  // namespace crawler_task_logic
