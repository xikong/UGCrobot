/*
 * robot_check_header.h
 *
 *  Created on: 2016年7月18日
 *      Author: harvey
 */

#ifndef KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_HEADER_H_
#define KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_HEADER_H_

#include <string.h>
#include <iostream>
#include <sstream>
#include <iostream>

#include "basic/scoped_ptr.h"
#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "logic/base_values.h"
#include "storage/storage_controller_engine.h"
#include "logic/value_serializer.h"
#include "logic/xml_serializer.h"
#include "logic/json_serializer.h"
#include "logic/jsonp_serializer.h"
#include "logic/http_serializer.h"
#include "basic/basic_util.h"
#include "check/newmacro.h"

using std::string;
namespace robot_check_logic {

enum TASKSTAE{
    TASK_WAIT = 0,
    TASK_SEND = 1,
    TASK_RECV = 2,
    TASK_READY = 3,
    TASK_EXECUING = 4,
    TASK_STORAGE = 5,
    TASK_STORAGED = 6,
    TASK_EXECUED = 7,
    TASK_FAIL = 8,
    TASK_SUCCESS = 9
};

typedef struct robot_task{
    int64 task_id_;
    int32 task_type_;
    string url_;
    string content_;
    int is_sucess_;

    void ValueSerialize(base_logic::DictionaryValue *dict ) {
        dict->GetBigInteger(L"id", &task_id_);
        dict->GetInteger(L"type", &task_type_);
        dict->GetString(L"url", &url_);
        dict->GetString(L"unicode", &content_);
        is_sucess_ = TASK_FAIL;
    }

} RobotTask;

}

#endif /* KID_PLUGINS_ROBOT_CHECK_ROBOT_CHECK_HEADER_H_ */
