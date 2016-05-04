/*
 * auto_task.cc
 *
 *  Created on: 2016年4月27日
 *      Author: Harvey
 */

#include "task_engine.h"

using std::string;
using logic::SomeUtils;

namespace base_logic {

bool TaskEngine::StartTaskWork(const string &str_referer,
        const string &str_content, string &str_response){

    bool r = false;

    //提交curl
    r = SendHttpRequestCurl(str_referer, str_response);
    if(!r){
        return false;
    }

    //判断执行结果
    r = this->JudgeResultByResponse(str_content, str_response);
    if(!r){
        return false;
    }

    return true;
}

bool TaskEngine::SendHttpRequestCurl(const string &str_url, string &str_response) {

    CURL* curl = NULL;
    struct curl_slist* headers = NULL;
    CURLcode res;
    curl = curl_easy_init();
    if (curl != NULL) {

        string str_ua = "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:45.0) Gecko/20100101 Firefox/45.0";

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); //设定为不验证证书和host
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
        curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //跳转  301  302
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, str_ua.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5); // 接收超时时间
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
        curl_easy_setopt(curl, CURLOPT_POST, false);

        headers = curl_slist_append(headers, "Cache-Control: max-age=0");
        headers = curl_slist_append(headers, "Accept-Charset: uft-8");
        headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
        headers = curl_slist_append(headers, "Connection: keep-alive");

        //伪造UA
        str_ua = "User-Agent: " + str_ua;
        headers = curl_slist_append(headers, str_ua.c_str());

        //伪造ip
//        if(!task->forge_ip_.empty()){
//          LOG_MSG2("fore_ip = %s", task->forge_ip_.c_str());
//            task->forge_ip_ = "REMOTE_ADDR: " + task->forge_ip_ + ", X-Forwarded-For: " + task->forge_ip_;
//            headers = curl_slist_append(headers, task->forge_ip_.c_str());
//        }


        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&(str_response));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, TaskEngine::ReadResponse);

        //如果超时，再提交一次
        res = curl_easy_perform(curl);
        if(CURLcode::CURLE_OPERATION_TIMEDOUT == res){
            res = curl_easy_perform(curl);
        }

        LOG_MSG2("url = %s, res = %d, error = %s", str_url.c_str(), res, curl_easy_strerror(res));
        curl_easy_cleanup(curl);

        return true;
    }

    return false;
}

size_t TaskEngine::ReadResponse(void* buffer, size_t size, size_t member, void* res) {
    size_t ressize = size*member;
    *((std::string*)res) += std::string((char*)buffer, ressize);
    return ressize;
}

TaskTieBaEngine *TaskTieBaEngine::instance_ = NULL;
bool TaskTieBaEngine::JudgeResultByResponse(
        const string &str_content, const string response){

    if(response.find(str_content) == string::npos){
        return false;
    }

    return true;
}

} /* namespace base_logic */
