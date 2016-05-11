//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry
#include <mysql.h>
#include "manager/manager_db.h"
#include "storage/storage.h"
#include "storage/storage_controller_engine.h"

namespace manager_logic {

ManagerDB::ManagerDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
}

ManagerDB::~ManagerDB() {
}

bool ManagerDB::CrawlerManagerLogin(void* data) {
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    base_logic::CrawlerScheduler* scheduler =
            (base_logic::CrawlerScheduler*)(data);
    std::string sql = "call proc_CrawlerManagerLogin(\'"+ scheduler->mac()
            + "\',\'" + scheduler->password() + "\');";
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*)(dict.get()),
            CallBackManagerLogin);
    if (!r)
        return r;
    int32 manager_id = 0;
    r = dict->GetInteger(L"manager_id", &manager_id);
    if (r) {
        scheduler->set_id(manager_id);
    }
    return true;
}

void  ManagerDB::CallBackManagerLogin(void* param,
        base_logic::Value* value) {
    base_logic::DictionaryValue* dict =
            (base_logic::DictionaryValue*)(value);

    base_storage::DBStorageEngine* engine =
            (base_storage::DBStorageEngine*)(param);
    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)) {
            if (rows[0] != NULL)
                dict->SetInteger(L"manager_id", atol(rows[0]));
        }
    }
}
}  // namespace manager_logic



