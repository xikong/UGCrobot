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
    TaskEngine() {}

 public:
    virtual ~TaskEngine() {}

    //发送请求
    virtual bool StartTaskWork(struct TaskHead *task, string &str_response);
    //组装参数
    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer) = 0;
    //判断结果
    virtual bool JudgeResultByResponse(string &response, string &code_num) = 0;
    //发送请求
    bool SendHttpRequestCurl(struct TaskHead *task, string &str_url,
                             string &str_postarg, string &str_referer,
                             string &str_response);
    //组装请求头
    void HandlerRequestHeader(struct curl_slist* headers);
    static size_t ReadResponse(void* buffer, size_t size, size_t member,
                               void* res);
    bool FindStrFromString(string &find, const string &src, const string start,
                           const char end);
 private:
    virtual void AssembleRequestHeader(struct curl_slist* headers) {}

};

//QQ
class TaskQQEngine : public TaskEngine {
 public:
    static TaskQQEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskQQEngine();
        }
        return instance_;
    }
    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    bool GetQQGtkByCookie(string &str_gtk, const string cookie);

 private:
    static TaskQQEngine *instance_;
};

//天涯
class TaskTianYaEngine : public TaskEngine {
 public:
    static TaskTianYaEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskTianYaEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    static TaskTianYaEngine *instance_;
};

//贴吧
class TaskTieBaEngine : public TaskEngine {
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

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    //关注贴吧
    bool TryAttentionKw(struct TaskTieBaPacket *task);
    //签到
    bool TrySignInKw(struct TaskTieBaPacket *task);
    //私信
    bool PostTieBaMessage(struct TaskTieBaPacket *task);
    virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
    static TaskTieBaEngine *instance_;
};

//微博
class TaskWeiBoEngine : public TaskEngine {
 public:
    static TaskWeiBoEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskWeiBoEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
    static TaskWeiBoEngine * instance_;
};

//猫扑
class TaskMopEngine : public TaskEngine {
 public:
    static TaskMopEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskMopEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    static TaskMopEngine *instance_;

};

//豆瓣
class TaskDouBanEngine : public TaskEngine {
 public:
    static TaskDouBanEngine *GetIntance() {
        if (NULL == instance_) {
            instance_ = new TaskDouBanEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    static TaskDouBanEngine *instance_;
};

//淘股吧
class TaskTaoGuBaEngine : public TaskEngine {
 public:
    static TaskTaoGuBaEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskTaoGuBaEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
    static TaskTaoGuBaEngine *instance_;

};

//雪球
class TaskXueQiuEngine : public TaskEngine {

 public:
    static TaskXueQiuEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskXueQiuEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:

    bool GetSessionToken(struct TaskXueQiuPacket *task,
                         string &str_session_token);
    bool GetCurrReplyId(const string &url, string &topic_id);
    virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
    static TaskXueQiuEngine *instance_;
};

//东方股吧
class TaskIGuBaEngine : public TaskEngine {
 public:
    static TaskIGuBaEngine *GetInstance() {
        if (NULL == instance_) {
            instance_ = new TaskIGuBaEngine();
        }
        return instance_;
    }

    static void FreeInstance() {
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    virtual void AssembleRequestHeader(struct curl_slist *headers);

 private:
    static TaskIGuBaEngine *instance_;
};

//同花顺
class TaskTongHuaShunEngine : public TaskEngine{
 public:
    static TaskTongHuaShunEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskTongHuaShunEngine();
        }
        return instance_;
    }

    static void FreeInstance(){
        delete instance_;
        instance_ = NULL;
    }

    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer);
    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    virtual void AssembleRequestHeader(struct curl_slist *headers);
    string GetUTime();
    bool GetPid(struct TaskTongHuaShunPacket &task_ths, string &str_pid);

 private:
    static TaskTongHuaShunEngine *instance_;
};

} /* namespace base_logic */

#endif /* KID_PUB_LOGIC_TASK_ENGINE_H_ */
