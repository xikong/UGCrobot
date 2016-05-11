//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015.9.22 Author: kerry


#include <mysql.h>
#include <sstream>
#include <set>

#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "storage/storage.h"
#include "storage/storage_controller_engine.h"
#include "basic/basic_util.h"
#include "basic/template.h"
#include "basic/radom_in.h"
#include "crawler_task_db.h"
#include "task_schduler_engine.h"

namespace tieba_task_logic {

CrawlerTaskDB::CrawlerTaskDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
}

CrawlerTaskDB::~CrawlerTaskDB() {
}

bool CrawlerTaskDB::FectchBatchForgeryUA(std::list<base_logic::ForgeryUA>* list) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                    new base_logic::DictionaryValue());
    std::string sql = "call proc_FecthBatchUA()";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackFectchBatchForgeryUA);
    if (!r)
        return r;
    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        base_logic::ForgeryUA ua;
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*)(result_value);
        ua.ValueSerialization(dict_result_value);
        list->push_back(ua);
        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
}

void CrawlerTaskDB::CallBackFectchBatchForgeryUA(void* param,
            base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
            (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetString(L"ua", rows[1]);
            if (rows[2] != NULL)
                info_value->SetInteger(L"type", atoll(rows[2]));
            if (rows[3] != NULL)
                info_value->SetString(L"create_time", rows[3]);
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::FecthBatchTask(std::list<base_logic::TiebaTask>* list,
        const bool is_new) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                new base_logic::DictionaryValue());

    std::string sql;
    if (is_new)
        sql  = "call proc_FecthNewTask()";
    else
        sql = "call proc_FecthBatchTask()";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackFectchBatchTask);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
	while (listvalue->GetSize()) {
		base_logic::TiebaTask task;
		base_logic::Value* result_value;
		listvalue->Remove(0, &result_value);
		base_logic::DictionaryValue* dict_result_value =
			(base_logic::DictionaryValue*)(result_value);
		task.ValueSerialization(dict_result_value);
		list->push_back(task);
		delete dict_result_value;
		dict_result_value = NULL;
	}

    return true;
}

void CrawlerTaskDB::CallBackFectchBatchTask(void* param,
            base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
            (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetBigInteger(L"attrid", atoll(rows[1]));
            if (rows[2] != NULL)
                info_value->SetCharInteger(L"depth",
                        logic::SomeUtils::StringToIntChar(rows[2]));
            if (rows[3] != NULL)
                info_value->SetCharInteger(L"machine",
                        logic::SomeUtils::StringToIntChar(rows[3]));
            if (rows[4] != NULL)
                info_value->SetCharInteger(L"storage",
                        logic::SomeUtils::StringToIntChar(rows[4]));
            if (rows[5] != NULL)
                info_value->SetCharInteger(L"islogin",
                        logic::SomeUtils::StringToIntChar(rows[5]));
            if (rows[6] != NULL)
                info_value->SetCharInteger(L"isforge",
                        logic::SomeUtils::StringToIntChar(rows[6]));
            if (rows[7] != NULL)
                info_value->SetCharInteger(L"isover",
                        logic::SomeUtils::StringToIntChar(rows[7]));
            if (rows[8] != NULL)
                info_value->SetCharInteger(L"method",
                        logic::SomeUtils::StringToIntChar(rows[8]));
            if (rows[9] != NULL)
                info_value->SetBigInteger(L"polling_time", atoll(rows[9])/2);
            if (rows[10] != NULL)
                info_value->SetString(L"url", rows[10]);
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::FetchLastTask(base_logic::TiebaTask &task) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                new base_logic::DictionaryValue());

    std::stringstream os;
    std::string sql;
    os << "call proc_FetchLastTask(\'" << task.url() << "\')";
    sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
    		CallBackFetchLastTask);
    if (!r)
        return false;
    int64 tmp;
    int32 state;
    dict->GetBigInteger(L"id", &tmp);
    task.set_id(tmp);
    dict->GetBigInteger(L"type", &tmp);
    task.set_type(tmp);
    dict->GetInteger(L"state", &state);
    task.set_state(state);
    dict->GetBigInteger(L"last_time", &tmp);
    task.update_time(tmp);
    return true;
}

void CrawlerTaskDB::CallBackFetchLastTask(void* param,
		base_logic::Value* value) {
	base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
	base_logic::ListValue* list = new base_logic::ListValue();
	base_storage::DBStorageEngine* engine =
			(base_storage::DBStorageEngine*) (param);
	MYSQL_ROW rows;
	int32 num = engine->RecordCount();
	if (num > 0) {
		if (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
			if (rows[0] != NULL)
				dict->SetBigInteger(L"id", atoll(rows[0]));
			if (rows[1] != NULL)
				dict->SetBigInteger(L"type", atoll(rows[1]));
			if (rows[2] != NULL)
				dict->SetInteger(L"state", atoi(rows[2]));
			if (rows[3] != NULL)
				dict->SetBigInteger(L"last_time", atoll(rows[3]));
		}
	}
}

bool CrawlerTaskDB::ExistMainpageTask(const std::string &rowkey, int &affected_rows) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                    new base_logic::DictionaryValue());
    std::stringstream os;
    os << "call proc_ExistMainpageTask(\'" << rowkey.c_str() << "\')";
    std::string sql = os.str();

    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
    		CallBackAffectedRow);
    if (!r)
        return false;
    dict->GetInteger(L"affected_row", &affected_rows);
    return true;
}

void CrawlerTaskDB::CallBackAffectedRow(void* param, base_logic::Value* value) {
	base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
	base_storage::DBStorageEngine* engine =
			(base_storage::DBStorageEngine*) (param);
	MYSQL_ROW rows;
	int32 num = engine->RecordCount();
	dict->SetInteger(L"affected_row", num);
	LOG_DEBUG2("affected rows: %d", num);
}
bool CrawlerTaskDB::GetCookies(int count, uint64 last_time, std::list<base_logic::LoginCookie>* cookies_list) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
	std::stringstream os;
	os << "call proc_GetRobotCookies(" << count << "," << last_time << ")";
    std::string sql = os.str();

    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackGetCookies);
    if (!r)
        return r;

    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        base_logic::LoginCookie login_cookie;
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*)(result_value);

        login_cookie.ValueSerialization(dict_result_value);
        login_cookie.update_send_time(0);
        cookies_list->push_back(login_cookie);
        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
}


void CrawlerTaskDB::CallBackGetCookies(void* param,
        base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
        (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
        (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    int64 attr_id;
    TaskSchdulerManager* schduler_manager = TaskSchdulerEngine::GetTaskSchdulerManager();
    if (num > 0) {
        while ( rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc) ) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"cookie_id", atoll(rows[0]));
            if (rows[1] != NULL) {
                attr_id = atoll(rows[1]);
                info_value->SetBigInteger(L"cookie_attr_id", attr_id);
            }
            if (rows[2] != NULL) {
                int64 last_time = atoll(rows[2]);
                info_value->SetBigInteger(L"last_time", last_time);
                int64& last_update_time =
                    schduler_manager->GetDatabaseUpdateTimeByPlatId(attr_id);
                if (last_time > last_update_time)
                    last_update_time = last_time;
            }
            if (rows[3] != NULL)
                info_value->SetString(L"cookie_body", rows[3]);

            if (rows[4] != NULL)
                info_value->SetString(L"username", rows[4]);

            if (rows[5] != NULL)
                info_value->SetString(L"passwd", rows[5]);

            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::FectchBatchForgeryIP(std::list<base_logic::ForgeryIP>* list) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                    new base_logic::DictionaryValue());
    std::string sql = "call proc_FecthBatchIP()";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackFectchBatchForgeryIP);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        base_logic::ForgeryIP ip;
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*)(result_value);
        ip.ValueSerialization(dict_result_value);
        list->push_back(ip);
        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
}

void CrawlerTaskDB::CallBackFectchBatchForgeryIP(void* param,
            base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
            (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetString(L"ip", rows[1]);
            if (rows[2] != NULL)
                info_value->SetCharInteger(L"type",
                        logic::SomeUtils::StringToIntChar(rows[2]));
            if (rows[3] != NULL)
                info_value->SetString(L"create_time", rows[3]);
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::RecordRobotTaskState(base_logic::RobotTask &task) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                new base_logic::DictionaryValue());
    std::string sql;
    std::stringstream os;
    os << "call proc_RecordMainPageTask(\'" << task.row_key() << "\', "
    		<< (int)task.type() << ",\'" << task.SerializeSelf() << "\', " << task.state() << ")";
    sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
    		CallBackGetTaskId);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
	while (listvalue->GetSize()) {
		base_logic::Value* result_value;
		listvalue->Remove(0, &result_value);
		base_logic::DictionaryValue* dict_result_value =
			(base_logic::DictionaryValue*)(result_value);
		task.ValueSerialization(dict_result_value);
//		task.set_type(MAIN_LASTING_TASK);
		delete dict_result_value;
		dict_result_value = NULL;
	}
    return true;
}

void CrawlerTaskDB::CallBackGetTaskId(void *param, base_logic::Value *value) {
    base_logic::DictionaryValue* dict =
            (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"id", atoll(rows[0]));
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::UpdateRobotTaskState(int64 task_id, int state) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                new base_logic::DictionaryValue());
    std::string sql;
    std::stringstream os;
    os << "call proc_UpdateMainpageTask(" << task_id << "," << state << ")";
    sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->WriteData(0, (base_logic::Value*)(dict.get()));
    if (!r)
        return false;
    return true;
}

int CrawlerTaskDB::GetCrawlerTypeByOpCode(uint32 op_code) {
    bool r = true;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    std::stringstream os;
    os << "call proc_GetCrawlerTypeByOpCode(" << op_code << ");";
    dict->SetString(L"sql", os.str());
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackGetCrawlerType);
    if (!r)
        return -1;
    int crawler_type = -1;
    r = dict->GetInteger(L"crawler_type", &crawler_type);
    return crawler_type;
}

void CrawlerTaskDB::CallBackGetCrawlerType(void *param, base_logic::Value *value) {
	base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
	base_storage::DBStorageEngine* engine =
			(base_storage::DBStorageEngine*) (param);
	MYSQL_ROW rows;
	int32 num = engine->RecordCount();
	if (num > 0) {
		while (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
			if (rows[0] != NULL)
				dict->SetInteger(L"crawler_type", atol(rows[0]));
		}
	}
}
}  // namespace crawler_task_logic
