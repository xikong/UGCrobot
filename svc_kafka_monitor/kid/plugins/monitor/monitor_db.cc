/*
 * monitor_db.cc
 *
 *  Created on: 2016年6月17日
 *      Author: Harvey
 */

#include "monitor_db.h"
#include <mysql/mysql.h>

namespace monitor_logic {

MonitorDB::MonitorDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
}

bool MonitorDB::FetchNewKafkaTopic(std::list<KafkaInfo *> *list, int is_first){

    bool r = false;
    std::stringstream os;
    scoped_ptr<base_logic::DictionaryValue> dict(new base_logic::DictionaryValue());
    if( is_first ){
        os << "call proc_FetchNewKafkaRecord(" << 0 <<  ")";
    }else{
        os << "call proc_FetchNewKafkaRecord(" << 1 <<  ")";
    }
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", os.str());
    r = mysql_engine_->ReadData(0, (base_logic::Value*) (dict.get()),
                                CallbackFetchNewKafkaTopic);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        KafkaInfo *kafka_info = new KafkaInfo();
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*) (result_value);
        kafka_info->ValueSerialize(dict_result_value);
        list->push_back(kafka_info);
        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
}

void MonitorDB::CallbackFetchNewKafkaTopic(void* param,
                base_logic::Value* value){
    base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine = (base_storage::DBStorageEngine*) (param);

    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {

        while (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetString(L"topic", (rows[1]));
            if (rows[2] != NULL)
                info_value->SetString(L"addr", (rows[2]));
            if (rows[2] != NULL)
                info_value->SetString(L"total_num", (rows[2]));

            list->Append((base_logic::Value*) (info_value));
        }
    }

    dict->Set(L"resultvalue", (base_logic::Value*) (list));
}

bool MonitorDB::RecordKafkaTaskNum(KafkaInfo *kafka_info){
    bool r = false;
    std::stringstream os;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    os << "call proc_RecordKafkaTaskNum(" << kafka_info->id_ << "," << kafka_info->total_num_ << ")";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", os.str());
    mysql_engine_->WriteData(0, (base_logic::Value*) (dict.get()));
    return true;
}


} /* namespace monitor_logic */
