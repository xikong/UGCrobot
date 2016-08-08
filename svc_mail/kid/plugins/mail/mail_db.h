/*
 * mail_db.h
 *
 *  Created on: 2016年6月15日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MAIL_MAIL_DB_H_
#define KID_PLUGINS_MAIL_MAIL_DB_H_

#include "mail/mail_header.h"

#include <string>
#include "storage/storage_controller_engine.h"
#include "logic/auto_crawler_infos.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace mail_logic {

class MailDB{
 public:
    MailDB();
    bool FetchBatchTaskContent(std::list<base_logic::MailContentInfo *> *list,
                               bool is_new = false);

    bool CheckMailTargetIsFirst(const string &mail_from, bool &is_exist);

    bool SaveComleteMailTask(const MailTask &mail_task);

 public:
    static void CallBackFetchBatchContent(void* param,
                base_logic::Value* value);

    static void CallBackCheckMailTargetIsFirst(void *param,
                base_logic::Value* value);

    static void CallBackSaveComleteMailTask(void *param,
                base_logic::Value* value);

 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};

} /* namespace mail_logic */

#endif /* KID_PLUGINS_MAIL_MAIL_DB_H_ */
