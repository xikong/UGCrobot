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

#include "net/comm_head.h"
#include "net/packet_define.h"
#include "net/packet_processing.h"
#include "logic/logic_unit.h"

using std::string;

namespace base_logic {

class TaskEngine {
 protected:
    TaskEngine() {};

 public:
    virtual ~TaskEngine() {};

    virtual bool StartTaskWork(struct TaskHead *task, string &str_response);

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer) = 0;

    virtual bool JudgeResultByResponse(string &response) = 0;

    bool SendHttpRequestCurl(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer, string &str_response);

    static size_t ReadResponse(void* buffer, size_t size, size_t member, void* res);

    bool FindStrFromString(string &find, const string &src,
            const string start, const char end);

};

class TaskQQEngine : public TaskEngine {
 public:
    static TaskQQEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskQQEngine();
        }
        return instance_;
    }
    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    bool GetQQGtkByCookie(string &str_gtk, const string cookie);

 private:
    static TaskQQEngine     *instance_;
};

class TaskTianYaEngine : public TaskEngine{
 public:
    static TaskTianYaEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskTianYaEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskTianYaEngine *instance_;
};

class TaskTieBaEngine : public TaskEngine{
 public:
    static TaskTieBaEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskTieBaEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskTieBaEngine  *instance_;
};

class TaskWeiBoEngine : public TaskEngine{
 public:
    static TaskWeiBoEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskWeiBoEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskWeiBoEngine *        instance_;
};

class TaskMopEngine : public TaskEngine{
 public:
    static TaskMopEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskMopEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskMopEngine *instance_;

};

class TaskDouBanEngine : public TaskEngine{
 public:
    static TaskDouBanEngine *GetIntance(){
        if(NULL == instance_){
            instance_ = new TaskDouBanEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskDouBanEngine *instance_;
};

class TaskTaoGuBaEngine : public TaskEngine{

 public:
    static TaskTaoGuBaEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskTaoGuBaEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer);

    virtual bool JudgeResultByResponse(string &response);

 private:
    static TaskTaoGuBaEngine *instance_;

};

} /* namespace base_logic */

#endif /* KID_PUB_LOGIC_TASK_ENGINE_H_ */
