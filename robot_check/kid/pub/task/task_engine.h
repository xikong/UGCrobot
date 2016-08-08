/*
 * auto_task.h
 *
 *  Created on: 2016年4月27日
 *      Author: Harvey
 */

#ifndef KID_PUB_LOGIC_TASK_ENGINE_H_
#define KID_PUB_LOGIC_TASK_ENGINE_H_

#include <sstream>
#include <iostream>
#include <curl/curl.h>
#include <stdlib.h>

#include "net/comm_head.h"
#include "net/packet_define.h"
#include "net/packet_processing.h"
#include "logic/logic_unit.h"

#include "../../plugins/robot_check/robot_check_header.h"
using robot_check_logic::RobotTask;

using std::string;

namespace base_logic {

class TaskEngine{
 protected:
    TaskEngine() {
    }
    ;

 public:
    virtual ~TaskEngine() {
    }
    ;

    virtual bool StartTaskWork(RobotTask &task );
    virtual bool JudgeResultByResponse(RobotTask &task, string &response ) = 0;
    bool SendHttpRequestCurl(const string &str_url, string &str_response );
    static size_t ReadResponse(void* buffer, size_t size, size_t member,
                               void* res );

};

class TaskTieBaEngine : public TaskEngine{
 public:
    static TaskTieBaEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskTieBaEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }
    virtual bool JudgeResultByResponse(RobotTask &task, string &response );

 private:
    static TaskTieBaEngine *instance_;
};

} /* namespace base_logic */

#endif /* KID_PUB_LOGIC_TASK_ENGINE_H_ */
