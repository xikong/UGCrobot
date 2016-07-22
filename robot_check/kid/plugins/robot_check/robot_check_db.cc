/*
 * robot_check_db.cc
 *
 *  Created on: 2016年7月18日
 *      Author: harvey
 */

#include "robot_check_db.h"
#include <mysql/mysql.h>

namespace robot_check_logic {

RobotCheckDB::RobotCheckDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
}

bool RobotCheckDB::FetchTodayTask(std::list<RobotTask *> &list,
                                  const string &check_date ) {
    bool r = false;
    std::stringstream os;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    os << "call proc_FetchRobotTask(\'" << check_date << "%" << "\')";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", os.str());
    r = mysql_engine_->ReadData(0, (base_logic::Value*) (dict.get()),
                                CallbackFetchTodayTask);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        RobotTask *robot_task = new RobotTask();
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*) (result_value);
        robot_task->ValueSerialize(dict_result_value);
        if (!robot_task->content_.empty()) {
            list.push_back(robot_task);
        } else {
            LOG_ERROR("LoadDBRobotTask, task_unicode empty");
            delete robot_task;
        }

        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
}

void RobotCheckDB::CallbackFetchTodayTask(void* param,
                                          base_logic::Value* value ) {
    base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*) (param);

    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetInteger(L"type", atoi(rows[1]));
            if (rows[2] != NULL)
                info_value->SetString(L"url", (rows[2]));
            if (rows[3] != NULL)
                info_value->SetString(L"unicode", (rows[3]));

            list->Append((base_logic::Value*) (info_value));
        }
    }

    dict->Set(L"resultvalue", (base_logic::Value*) (list));
}

bool RobotCheckDB::RecordRobotTaskStatus(RobotTask &task ) {
    std::stringstream os;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    os << "call proc_UpdateRobotTaskStatus(" << task.task_id_ << ","
       << task.is_sucess_ << ")";
    dict->SetString(L"sql", os.str());
    mysql_engine_->WriteData(0, (base_logic::Value*) (dict.get()));
    return true;
}

} /* namespace robot_check_logic */
