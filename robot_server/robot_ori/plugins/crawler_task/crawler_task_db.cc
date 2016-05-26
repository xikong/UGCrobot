//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015.9.22 Author: kerry

#include <mysql.h>
#include <sstream>
#include <set>
#include "crawler_task/crawler_task_db.h"
#include "crawler_task/task_schduler_engine.h"
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "storage/storage.h"
#include "storage/storage_controller_engine.h"
#include "basic/basic_util.h"
#include "basic/template.h"
#include "basic/radom_in.h"

namespace crawler_task_logic {

CrawlerTaskDB::CrawlerTaskDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
    task_platform_inited_ = false;
}

CrawlerTaskDB::~CrawlerTaskDB() {
}

bool CrawlerTaskDB::RecordRobotTaskResult(int64 task_id, int is_success) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(new base_logic::DictionaryValue());
	std::stringstream os;
	os << "call proc_RecordRobotTaskResult(" << task_id << "," << is_success << ")";
    std::string sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            NULL);
    if (!r)
        return false;
    return true;
}

bool CrawlerTaskDB::GetCookie(std::list<base_logic::LoginCookie>* cookie_list, const int64 id,
            const int64 from, const int64 count, int64& plat_update_time) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    std::string sql;
    sql = "call proc_GetCookieV4("+base::BasicUtil::StringUtil::Int64ToString(id)
        +","+base::BasicUtil::StringUtil::Int64ToString(count)
        +","+base::BasicUtil::StringUtil::Int64ToString(plat_update_time)
        +");";

    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackGetCookies);
    if (!r)
        return r;

    dict->GetList(L"resultvalue", &listvalue);
    LOG_MSG2("listvalue->GetSize() %d", listvalue->GetSize());
    while (listvalue->GetSize()) {
        base_logic::LoginCookie login_cookie;
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*)(result_value);

        login_cookie.ValueSerialization(dict_result_value);
        login_cookie.set_is_read(false);
        cookie_list->push_back(login_cookie);
        delete dict_result_value;
        dict_result_value = NULL;
    }
    return true;
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

void CrawlerTaskDB::CallBackGetCookie(void* param,
        base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
        (base_logic::DictionaryValue*)(value);
    base_logic::ListValue* list = new base_logic::ListValue();
    base_storage::DBStorageEngine* engine =
        (base_storage::DBStorageEngine*)(param);

    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while ( rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            base_logic::DictionaryValue* info_value =
                    new base_logic::DictionaryValue();
            if (rows[0] != NULL)
                info_value->SetBigInteger(L"cookie_attr_id", atoll(rows[0]));
            if (rows[1] != NULL)
                info_value->SetString(L"cookie_body", rows[1]);

            if (rows[2] != NULL)
                info_value->SetString(L"username", rows[2]);

            if (rows[3] != NULL)
                info_value->SetString(L"passwd", rows[3]);

            /*
            if (rows[2] != NULL)
                info_value->SetString(L"last_time", rows[2]);
            */
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
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

int CrawlerTaskDB::RecordWeiboTaskState(base_logic::WeiboTaskInfo &task) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(new base_logic::DictionaryValue());
	std::stringstream os;
	os << "call proc_RecordWeiboTaskState(\'" << task.topic_id() << "\',\'" << task.host_uin() << "\',\'" << task.content() << "\')";
    std::string sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackRecordWeiboTaskState);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
	while (listvalue->GetSize()) {
		base_logic::Value* result_value;
		listvalue->Remove(0, &result_value);
		base_logic::DictionaryValue* dict_result_value =
			(base_logic::DictionaryValue*)(result_value);
		task.ValueSerialization(dict_result_value);
		delete dict_result_value;
		dict_result_value = NULL;
	}
    return true;
}

void CrawlerTaskDB::CallBackRecordWeiboTaskState(void* param,
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
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::FetchRobotCookies(std::list<struct RobotCookie> *list, int64 attr_id, int count) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(new base_logic::DictionaryValue());
	std::stringstream os;
	os << "call proc_GetRobotCookie(" << attr_id << "," << count << ")";
    std::string sql = os.str();
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackFetchRobotCookies);
    if (!r) {
		LOG_MSG("query sql error");
        return false;
	}
    dict->GetList(L"resultvalue", &listvalue);
	if (listvalue->GetSize() != count) {
		LOG_MSG2("I want to get %d cookies, but real get %d cookies", 
				count, listvalue->GetSize());
	}
	while (listvalue->GetSize()) {
		struct RobotCookie cookie;
		base_logic::Value* result_value;
		listvalue->Remove(0, &result_value);
		base_logic::DictionaryValue* dict_result_value =
			(base_logic::DictionaryValue*)(result_value);
		dict_result_value->GetBigInteger(L"id", &cookie.id);
		dict_result_value->GetString(L"cookie", &cookie.cookie);
		list->push_back(cookie);
		delete dict_result_value;
		dict_result_value = NULL;
	}
    return true;
}

void CrawlerTaskDB::CallBackFetchRobotCookies(void* param,
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
                info_value->SetString(L"cookie", rows[1]);
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

void CrawlerTaskDB::CallBackFetchRobotIP(void* param,
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
                info_value->SetString(L"ip", rows[0]);
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

bool CrawlerTaskDB::FecthBatchTask(std::list<base_logic::TaskInfo>* list,
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
		base_logic::TaskInfo task;
		base_logic::Value* result_value;
		listvalue->Remove(0, &result_value);
		base_logic::DictionaryValue* dict_result_value =
			(base_logic::DictionaryValue*)(result_value);
		task.ValueSerialization(dict_result_value);
		task.set_type(MAIN_LASTING_TASK);
		list->push_back(task);
		delete dict_result_value;
		dict_result_value = NULL;
	}

    return true;
}

bool CrawlerTaskDB::RecordTaskState(
    base_logic::TaskInfo& task, const int32 type) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                new base_logic::DictionaryValue());
    std::string sql;
    sql = "call proc_RecordTaskState("
         +base::BasicUtil::StringUtil::Int64ToString(task.id())
         +","+base::BasicUtil::StringUtil::Int64ToString(static_cast<int64>(task.state()))
         +","+base::BasicUtil::StringUtil::Int64ToString(task.last_task_time())
         +","+base::BasicUtil::StringUtil::Int64ToString((task.polling_time()))
         +","+base::BasicUtil::StringUtil::Int64ToString(time(NULL))
         +","+base::BasicUtil::StringUtil::Int64ToString(type)+")";
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

bool CrawlerTaskDB::GetTaskPlatTaskDescription(
        std::list<base_logic::TaskPlatDescription>* list) {
    //  call crawler.proc_GetTaskPlatInfo()
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
                        new base_logic::DictionaryValue());
    std::string sql = "call proc_GetTaskPlatInfo()";
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackGetTaskPlatDescription);
    if (!r)
        return false;

    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        base_logic::TaskPlatDescription description;
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        description.ValueSerialization(
                (base_logic::DictionaryValue*)(result_value));
        list->push_back(description);
    }
    return true;
}

void CrawlerTaskDB::CallBackGetTaskPlatDescription(void* param,
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
                info_value->SetCharInteger(L"depth",
                    logic::SomeUtils::StringToIntChar(rows[1]));
            if (rows[2] != NULL)
                info_value->SetCharInteger(L"machine",
                    logic::SomeUtils::StringToIntChar(rows[2]));
            if (rows[3] != NULL)
                info_value->SetCharInteger(L"storage",
                    logic::SomeUtils::StringToIntChar(rows[3]));
            if (rows[4] != NULL)
                info_value->SetCharInteger(L"isforge",
                    logic::SomeUtils::StringToIntChar(rows[4]));
            if (rows[5] != NULL)
                info_value->SetCharInteger(L"isover",
                    logic::SomeUtils::StringToIntChar(rows[5]));
            if (rows[6] != NULL)
                info_value->SetString(L"description", rows[6]);
            list->Append((base_logic::Value*)(info_value));
        }
    }
    dict->Set(L"resultvalue", (base_logic::Value*)(list));
}

void CrawlerTaskDB::BatchFectchTaskPlatInfo(
        std::list<base_logic::TaskPlatDescription>* list) {
    while ((*list).size() > 0) {
        base_logic::TaskPlatDescription info = (*list).front();
        (*list).pop_front();
        task_platform_[info.id()] = info;
    }
}

void CrawlerTaskDB::BatchUpdateTaskInfo(
        std::list<base_logic::TaskInfo>* list) {
    if (!task_platform_inited_) {
        std::list<base_logic::TaskPlatDescription> list;
        GetTaskPlatTaskDescription(&list);
        BatchFectchTaskPlatInfo(&list);
        task_platform_inited_ = true;
    }
    std::list<base_logic::TaskInfo>::iterator it =
            (*list).begin();
    for (; it != (*list).end(); it++) {
        bool r = false;
        base_logic::TaskInfo info = (*it);
        base_logic::TaskPlatDescription  descripition;
        r = base::MapGet<TASKPLAT_MAP, TASKPLAT_MAP::iterator,
                int64, base_logic::TaskPlatDescription>(
                  task_platform_,
                  info.attrid(), descripition);
        if (r) {
            info.set_is_forge(descripition.forge());
            info.set_machine(descripition.machine());
            info.set_storage(descripition.storage());
            info.set_is_over(descripition.over());
        } else {
            //  若id不存在，则需到数据库获取 是否有新的
            continue;
        }
    }
}

}  // namespace crawler_task_logic