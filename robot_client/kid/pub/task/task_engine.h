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
    TaskEngine() {
    	InitThreadrw(&lock_);
    };

 public:
    virtual ~TaskEngine() {
    	DeinitThreadrw(lock_);
    };

    //发送请求
    virtual bool StartTaskWork(struct TaskHead *task, string &str_response);

    //组装请求体
    virtual bool HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer) = 0;

    //判断结果
    virtual bool JudgeResultByResponse(string &response, string &code_num) = 0;

    //发送请求
    bool SendHttpRequestCurl(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer, string &str_response);

    //组装请求头
    void HandlerRequestHeader(struct curl_slist* headers);

    static size_t ReadResponse(void* buffer, size_t size, size_t member, void* res);

    bool FindStrFromString(string &find, const string &src,
            const string start, const char end);

 private:
    virtual void AssembleRequestHeader(struct curl_slist* headers){};

 private:
    struct threadrw_t*  lock_;

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
    //关注贴吧
    bool TryAttentionKw(struct TaskTieBaPacket *task);

    //签到
    bool TrySignInKw(struct TaskTieBaPacket *task);

    virtual void AssembleRequestHeader(struct curl_slist* headers);

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

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

    virtual bool JudgeResultByResponse(string &response, string &code_num);

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

	virtual bool JudgeResultByResponse(string &response, string &code_num);

 private:
	virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
	static TaskTaoGuBaEngine *instance_;

};

class TaskXueQiuEngine : public TaskEngine{

 public:
	static TaskXueQiuEngine *GetInstance(){
		if(NULL == instance_){
			instance_ = new TaskXueQiuEngine();
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

	bool GetSessionToken(struct TaskXueQiuPacket *task, string &str_session_token);

	bool GetCurrReplyId(const string &url, string &topic_id);

	virtual void AssembleRequestHeader(struct curl_slist* headers);

 private:
	static TaskXueQiuEngine *instance_;
};

class TaskIGuBaEngine : public TaskEngine{
 public:
    static TaskIGuBaEngine *GetInstance(){
        if(NULL == instance_){
            instance_ = new TaskIGuBaEngine();
        }
        return  instance_;
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

 private:
    static TaskIGuBaEngine *instance_;
};

} /* namespace base_logic */

#endif /* KID_PUB_LOGIC_TASK_ENGINE_H_ */
