/*
 * mail_db.cc
 *
 *  Created on: 2016年6月15日
 *      Author: Harvey
 */

#include "mail/mail_db.h"
#include <mysql/mysql.h>

namespace mail_logic {

MailDB::MailDB() {
    mysql_engine_.reset(base_logic::DataControllerEngine::Create(MYSQL_TYPE));
}

bool MailDB::FetchBatchTaskContent(std::list<base_logic::MailContentInfo *> *list,
                                   bool is_new ) {
    bool r = false;
    std::stringstream os;
    std::string sql;
    scoped_ptr<base_logic::DictionaryValue> dict(new base_logic::DictionaryValue());

    if (is_new) {
        os << "call proc_FetchBatchMailInfo(0)";
        sql = os.str();
    } else {
        sql = "call proc_FetchBatchMailInfo(1)";
    }

    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*) (dict.get()),
                                CallBackFetchBatchContent);
    if (!r)
        return false;
    dict->GetList(L"resultvalue", &listvalue);
    while (listvalue->GetSize()) {
        base_logic::MailContentInfo *content =
                new base_logic::MailContentInfo();
        base_logic::Value* result_value;
        listvalue->Remove(0, &result_value);
        base_logic::DictionaryValue* dict_result_value =
                (base_logic::DictionaryValue*) (result_value);
        content->ValueSerialize(dict_result_value);
        list->push_back(content);
        delete dict_result_value;
        dict_result_value = NULL;
    }

    return true;
}

void MailDB::CallBackFetchBatchContent(void* param, base_logic::Value* value ) {
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
                info_value->SetString(L"mail_subject", (rows[1]));
            if (rows[2] != NULL)
                info_value->SetString(L"mail_body", (rows[2]));

            list->Append((base_logic::Value*) (info_value));
        }
    }

    dict->Set(L"resultvalue", (base_logic::Value*) (list));
}

bool MailDB::CheckMailTargetIsFirst(const string &mail_from, bool &is_exist){
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    std::stringstream os;
    os << "call proc_FilterMailTask(\'" << mail_from << "\')";
    std::string sql = os.str();
    LOG_DEBUG2("MailDB::CheckMailTargetIsFirst Sql = %s", sql.c_str());
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*) (dict.get()),
                                CallBackCheckMailTargetIsFirst);
    if (!r)
      return false;

    int affected_row;
    r = dict->GetInteger(L"affected_row", &affected_row);
    if( r && affected_row > 0 ){
        is_exist = true;
        LOG_DEBUG("MailDB::CheckMailTargetIsFirst, Success");
        return true;
    }

    return false;
}

void MailDB::CallBackCheckMailTargetIsFirst(void* param, base_logic::Value* value){
    base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
    base_storage::DBStorageEngine* engine = (base_storage::DBStorageEngine*) (param);

    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
            if (rows[0] != NULL)
                dict->SetInteger(L"affected_row", atol(rows[0]));
        }
    }
}

bool MailDB::SaveComleteMailTask(const MailTask &mail_task){
    bool r = false;
    scoped_ptr<base_logic::DictionaryValue> dict(
            new base_logic::DictionaryValue());
    std::string sql;
    std::stringstream os;
    os << "call proc_RecordMailTask("
            << "\'" << mail_task.from_ << "\'" << ","
            << "\'" << mail_task.to_ << "\'" << ","
            << mail_task.mail_content_id_
            << ")";
    sql = os.str();
    LOG_DEBUG2("MailDB::SaveComleteMailTask Sql = %s", sql.c_str());
    base_logic::ListValue* listvalue;
    dict->SetString(L"sql", sql);
    r = mysql_engine_->ReadData(0, (base_logic::Value*) (dict.get()),
                                CallBackSaveComleteMailTask);
    if (!r)
        return false;
    int id;
    r = dict->GetInteger(L"id", &id);
    if( r && id > 0 ){
        LOG_DEBUG2("MailDB::SaveComleteMailTask Save Success, id = %d", id);
        return true;
    }

    return false;
}

void MailDB::CallBackSaveComleteMailTask(void* param, base_logic::Value* value){
    base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
    base_storage::DBStorageEngine* engine = (base_storage::DBStorageEngine*) (param);

    MYSQL_ROW rows;
    int32 num = engine->RecordCount();
    if (num > 0) {
        while (rows = (*(MYSQL_ROW*) (engine->FetchRows())->proc)) {
            if (rows[0] != NULL)
                dict->SetInteger(L"id", atol(rows[0]));
        }
    }
}

} /* namespace mail_logic */
