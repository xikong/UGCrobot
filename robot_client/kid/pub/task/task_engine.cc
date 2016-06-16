/*
 * auto_task.cc
 *
 *  Created on: 2016年4月27日
 *      Author: Harvey
 */

#include "task_engine.h"
#include <stdlib.h>
#include "task/task_fixed_param.h"

using std::string;

namespace base_logic {

bool TaskEngine::StartTaskWork(struct TaskHead *task, string &str_response) {

    bool r = false;

    //获取伪造的ip
    string forgery_ip;
    FindStrFromString(forgery_ip, task->forge_ip_, "", ':');
    task->forge_ip_ = forgery_ip;

    //构造提交参数, url, referer等
    string str_url, str_post, str_referer;
    r = this->HandlerPostArg(task, str_url, str_post, str_referer);
    if (!r) {
        task->is_success_ = TASK_FAIL;
        str_response = "参数非法";
        task->error_no_ = "参数非法";
        LOG_MSG2("task_type = %d, task_id = %d, HandlerPostArg Failed",
                task->task_type_, task->task_id_);
        return false;
    }

    //提交curl
    r = SendHttpRequestCurl(task, str_url, str_post, str_referer, str_response);
    if (!r) {
        LOG_MSG2("task_type = %d, task_id = %d, SendHttpRequestCurl Failed",
                task->task_type_, task->task_id_);
        return false;
    }

    //判断执行结果
    r = this->JudgeResultByResponse(str_response, task->error_no_);
    if (!r) {
        task->is_success_ = TASK_FAIL;
        return false;
    }

    return true;
}

bool TaskEngine::SendHttpRequestCurl(struct TaskHead *task, string &url,
                                     string &str_postarg, string &str_referer,
                                     string &str_response) {

    CURL* curl = NULL;
    struct curl_slist* headers = NULL;
    CURLcode res;
    curl = curl_easy_init();
    if (curl != NULL) {

        //如果UA为空，使用默认的UA
        if (task->forge_ua_.empty()) {
            task->forge_ua_ =
                    "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:45.0) Gecko/20100101 Firefox/45.0";
        }

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);  //设定为不验证证书和host
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);  //跳转  301  302
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, task->forge_ua_.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);  // 接收超时时间
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);

        if (!str_postarg.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, true);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_postarg.c_str()); LOG_MSG2("post_data = %s", str_postarg.c_str());
        }

        //固定头
        HandlerRequestHeader(headers);

        if(!str_postarg.empty()){
            std::stringstream os;
            os << "Content-Length: ";
            os << str_postarg.size();
            headers = curl_slist_append(headers, os.str().c_str());
        }

        //cookie
        string str_cookie = "Cookie: " + task->cookie_;
        headers = curl_slist_append(headers, str_cookie.c_str());

        //伪造UA
        string str_forge_ua = "User-Agent: " + task->forge_ua_;
        headers = curl_slist_append(headers, str_forge_ua.c_str());

        //伪造ip
        if (!task->forge_ip_.empty()) {
            string str_forge_ip = "X-Forwarded-For: " + task->forge_ip_;
            headers = curl_slist_append(headers, str_forge_ip.c_str());
        }

        //请求来源
        if(!str_referer.empty()){
            str_referer = "Referer: " + str_referer;
            headers = curl_slist_append(headers, str_referer.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void* )&(str_response));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, TaskEngine::ReadResponse);

        //如果超时
        int i = 0;
        res = curl_easy_perform(curl);
        while (res == CURLcode::CURLE_OPERATION_TIMEDOUT && i < 3) {
            res = curl_easy_perform(curl);
            ++i;
        }

        if (res != 0) {
            string curl_fail_msg = curl_easy_strerror(res);
            task->is_success_ = TASK_FAIL;
            task->error_no_ = curl_fail_msg;
            str_response = curl_fail_msg;
            LOG_MSG2("url = %s, res = %d, error = %s", url.c_str(), res, curl_fail_msg.c_str());
        }

        curl_easy_cleanup(curl);

        return true;
    }

    return false;
}

void TaskEngine::HandlerRequestHeader(struct curl_slist* headers) {

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "X-Requested-With: XMLHttpRequest");
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");

    this->AssembleRequestHeader(headers);
}

size_t TaskEngine::ReadResponse(void* buffer, size_t size, size_t member,
                                void* res) {
    size_t ressize = size * member;
    *((std::string*) res) += std::string((char*) buffer, ressize);
    return ressize;
}

bool TaskEngine::FindStrFromString(string &find, const string &src,
                                   const string start, const char end) {

    std::size_t ret = 0;
    if (!start.empty()) {
        //检查要查找的字符串中有没有起始字符串
        ret = src.find(start);
        if (ret == string::npos) {
            return false;
        }
    }

    //获取查找字符串之后的字符串
    ret = ret + start.size();
    string end_str = src.substr(ret);

    //如果要找到末尾，直接返回
    if ('\0' == end) {
        find = end_str;
        return true;
    }

    //判断剩余字符串有没有结束字符
    ret = end_str.find(end);
    if (ret == string::npos) {
        return false;
    }

    std::stringstream os;
    int end_total_size = end_str.size();
    const char *c_src = end_str.c_str();
    for (int i = 0; i < end_total_size; ++i) {
        if (c_src[i] == end) {
            break;
        }

        os << c_src[i];
    }

    find = os.str();

    return true;
}

TaskQQEngine *TaskQQEngine::instance_ = NULL;
bool TaskQQEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                  string &str_postarg, string &str_referer) {

    bool r = false;
    struct TaskQQPacket *qq_task = (struct TaskQQPacket *) task;
    if (NULL == qq_task) {
        return false;
    }

    //获取自己的QQ号
    string my_uin = "";
    r = FindStrFromString(my_uin, qq_task->cookie_, "uin=o0", ';');
    if (!r) {
        LOG_MSG("QQZone Task Find Uin Failed");
        return false;
    }

    //获取g_tk
    string g_tk;
    r = GetQQGtkByCookie(g_tk, qq_task->cookie_);
    if (!r) {
        return false;
    }

    //对方的QQ号
    char temp[sizeof(int64)];
    sprintf(temp, "%d", qq_task->host_uin_);
    string str_hostUin(temp);

    //组装提交post参数
    std::stringstream os;
    if (qq_task->topic_id_.empty()) {

        //QQ 留言板固定参数
        os << "qzreferrer=http%3A%2F%2Fctc.qzs.qq.com%2Fqzone%2Fmsgboard%2Fmsgbcanvas.html%23page%3D1";
        os << "&format=fs&inCharset=utf-8&outCharset=utf-8&iNotice=1&ref=qzone&json=1";

        str_url = "http://m.qzone.qq.com/cgi-bin/new/add_msgb?g_tk=" + g_tk;
        str_referer = "http://ctc.qzs.qq.com/qzone/msgboard/msgbcanvas.html";

    } else {

        //QQ 评论说说固定参数
        os << "qzreferrer=http%3A%2F%2Fuser.qzone.qq.com%2F";
        os << my_uin;
        os << "&feedsType=100&inCharset=utf-8&outCharset=utf-8&plat=qzone&source=ic&isSignIn=&platformid=52";
        os << "&format=fs&ref=feeds&richval=&richtype=&private=0&paramstr=2";
        os << "&topicId=" << qq_task->topic_id_;

        str_url =
                "http://taotao.qzone.qq.com/cgi-bin/emotion_cgi_re_feeds?g_tk="
                        + g_tk;
        str_referer = "http://user.qzone.qq.com/" + str_hostUin;
    }

    os << "&hostUin=" << qq_task->host_uin_;
    os << "&uin=" << my_uin;
    os << "&content=" << qq_task->content_;

    str_postarg = os.str();
}

bool TaskQQEngine::JudgeResultByResponse(string &response, string &code_num) {

    FindStrFromString(code_num, response, "\"code\":", ',');
    if (code_num != "0") {
        return false;
    }

    return true;
}

bool TaskQQEngine::GetQQGtkByCookie(string &str_gtk, const string cookie) {

    string p_skey;
    bool r = FindStrFromString(p_skey, cookie, "p_skey=", ';');
    if (!r) {
        LOG_MSG("GetGtk Failed");
        return false;
    }

    LOG_MSG2("p_skey = %s", p_skey.c_str());

    int hash = 5381;
    int len = p_skey.size();
    for (int i = 0; i < len; ++i) {
        hash += (hash << 5) + (int) p_skey[i];
    }

    int64 g_tk = hash & 0x7fffffff;

    std::stringstream os;
    os << g_tk;
    str_gtk = os.str();

    LOG_MSG2("str_gtk = %s", str_gtk.c_str());

    return true;
}

TaskTianYaEngine *TaskTianYaEngine::instance_ = NULL;
bool TaskTianYaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                      string &str_postarg,
                                      string &str_referer) {

    bool r = false;
    struct TaskTianYaPacket *tianya_task = (struct TaskTianYaPacket *) task;
    if (NULL == tianya_task) {
        return false;
    }

    //http://bbs.tianya.cn/post-water-1727544-1.shtml
    //获取板块名
    string bbs_item;
    r = FindStrFromString(bbs_item, tianya_task->pre_url_, "post-", '-');
    if (!r) {
        return false;
    }

    //获取帖子id
    string page_id;
    r = FindStrFromString(page_id, tianya_task->pre_url_, bbs_item + "-", '-');
    if (!r) {
        return false;
    }

    //组合post语句
    std::stringstream os;
    os << "params.artId=" << page_id;
    os << "&params.item=" << bbs_item;
    os << "&params.appBlock=" << bbs_item;
    os << "&params.postId=" << page_id;
    os << "&params.preUrl=" << tianya_task->pre_url_;
    os << "&params.preTitle=" << tianya_task->pre_title_;
    os << "&params.preUserId=" << tianya_task->pre_user_id_;
    os << "&params.preUserName=" << tianya_task->pre_user_name_;
    os << "&params.prePostTime=" << tianya_task->pre_post_time_;
    os << "&params.content=" << tianya_task->content_;
    os << "&params.title=" << tianya_task->content_;

    //此为固定参数
    os << "&params.appId=bbs&params.action=f5.1461132164747.6493%2C%2Cd9.49.112%2Cp10.49.1%";
    os << "2Cu11.49.63%2Cd12.49.96%2Cp13.49.1%2Cu14.49.79%2Cb15.9.2177%2C%2C%2C%2C%2C%2Cd2.123.1024";
    os << "%2Cu3.123.119%2Cb4.17.238%2Cd6.49.2519%2Cp7.49.1%2Cu8.49.79%7C65ee3a4407053548d5d00e8defee2289";
    os << "%7Cc4ca4238a0b923820dcc509a6f75849b%7CMozilla%2F5.0+(Windows+NT+10.0%3B+Win64%3B+x64)+AppleWebKit";
    os << "%2F537.36+(KHTML%2C+like+Gecko)+Chrome%2F49.0.2623.112+Safari%2F537.36%7Cv2.3.1";
    os << "&params.sourceName=%E5%A4%A9%E6%B6%AF%E8%AE%BA%E5%9D%9B&params.type=3";
    os << "&params.bScore=true&params.bWeiBo=false";

    str_postarg = os.str();
    str_url = "http://bbs.tianya.cn/api?method=bbs.ice.reply";
    str_referer = tianya_task->pre_url_;

    return true;
}

bool TaskTianYaEngine::JudgeResultByResponse(string &response,
                                             string &code_num) {

    FindStrFromString(code_num, response, "{\"success\":\"", '"');
    if (code_num == "1") {
        return true;
    }

    return false;
}

TaskTieBaEngine *TaskTieBaEngine::instance_ = NULL;
bool TaskTieBaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                     string &str_postarg, string &str_referer) {

    bool r = false;
    struct TaskTieBaPacket *tieba_task = (struct TaskTieBaPacket*) task;
    if (NULL == tieba_task) {
        return false;
    }

    //获得tid
    string str_tid;
    r = FindStrFromString(str_tid, tieba_task->pre_url_, "com/p/", '\0');
    if (!r) {
        LOG_MSG2("FindStrFromString TieBa Tid = %s failed", str_tid.c_str());
        return false;
    }

    //获取tbs
    r = FindStrFromString(tieba_task->tbs_, tieba_task->cookie_, "tbs=", ';');
    if (!r) {
        LOG_MSG2("FindStrFromString TieBa task_cookie = %s, tbs = %s failed",
                tieba_task->cookie_.c_str(), tieba_task->tbs_.c_str());
        return false;
    }

    //获取user_id
    r = FindStrFromString(tieba_task->user_id_, tieba_task->cookie_, "user_id=",
                          ';');
    if (!r) {
        LOG_MSG2("FindStrFromString TieBa task_cookie = %s, user_id = %s failed",
                tieba_task->cookie_.c_str(), task->user_id_.c_str());
        return false;
    }

    //关注本吧
    TryAttentionKw(tieba_task);

    //尝试签到
    TrySignInKw(tieba_task);

    //组装post语句
    std::stringstream os;

    if (tieba_task->repost_id_.empty()) {
        //固定参数
        os << TIEBA_FIXED_PARAM_REPLY;
        os << "&mouse_pwd_t=" << logic::SomeUtils::GetCurrentTimeMs();
        tieba_task->floor_num_ = 1;
    } else {
        //楼中楼回复固定参数
        os << TIEBA_FIXED_PARAM_REPLY_FLOOR;
        os << "&quote_id=" << tieba_task->repost_id_;
        os << "&repostid=" << tieba_task->repost_id_;
        tieba_task->floor_num_ = 2;
    }


    os << "&kw=" << tieba_task->kw_;
    os << "&fid=" << tieba_task->fid_;
    os << "&tid=" << str_tid;
    os << "&floor_num=" << tieba_task->floor_num_;
    os << "&tbs=" << tieba_task->tbs_;
    os << "&content=" << tieba_task->content_;

    str_postarg = os.str();
    str_url = TIEBA_FIXED_URL_COMMIT_POST_ADD;
    str_referer = tieba_task->pre_url_;

    return true;
}

void TaskTieBaEngine::AssembleRequestHeader(struct curl_slist* headers) {
    headers = curl_slist_append(headers, "Host: tieba.baidu.com");
}

bool TaskTieBaEngine::JudgeResultByResponse(string &response,
                                            string &code_num) {

    FindStrFromString(code_num, response, "no\":", ',');
    if (code_num == "0") {
        response = code_num;
        return true;
    }

    response = code_num;
    return false;
}

bool TaskTieBaEngine::TryAttentionKw(struct TaskTieBaPacket *task_tb) {

    bool r = false;

    //根据cookie获取uid
    string str_uid;
    r = FindStrFromString(str_uid, task_tb->cookie_, "BAIDUID=", ':');
    if (!r) {
        LOG_MSG2("Parse User uid failed, cookie = %s", task_tb->cookie_.c_str());
        return false;
    }

    std::stringstream os;
    os << "fid=" << task_tb->fid_;
    os << "&fname=" << task_tb->kw_;
    os << "&uid=" << str_uid;
    os << "&ie=gbk";
    os << "&tbs=" << task_tb->tbs_;

    string str_post = os.str();
    string str_referer = "http://tieba.baidu.com/f?kw=" + task_tb->kw_;
    string str_url = TIEBA_FIXED_URL_LIKE_KW;
    string str_response;

    SendHttpRequestCurl(task_tb, str_url, str_post, str_referer, str_response);

    return true;
}

bool TaskTieBaEngine::TrySignInKw(struct TaskTieBaPacket *task_tb) {

    bool r = false;

    std::stringstream os;
    os << "ie=utf-8&kw=" << task_tb->kw_;
    os << "&tbs=" << task_tb->tbs_;

    string str_post = os.str();
    string str_referer = task_tb->pre_url_;
    string str_url = TIEBA_FIXED_URL_SIGN_KW;
    string str_response;
    SendHttpRequestCurl(task_tb, str_url, str_post, str_referer, str_response);

    return true;
}

bool TaskTieBaEngine::PostTieBaMessage(struct TaskTieBaPacket *task_tb){

    bool r = false;

    std::stringstream os;
    time_t current_time = time(NULL);
    string str_response;
    string str_post = "";
    string str_referer = "";
    string str_url = TIEBA_MESSAGE_POST_URL;

    os << TIEBA_MESSAGE_HOME_URL << current_time;
    string str_message_main_page = os.str();
    os.str("");
    r = SendHttpRequestCurl(task_tb, str_message_main_page, str_post, str_referer, str_response);
    if(!r){
        LOG_MSG("PostTieBaMessage Get BdsToken Failed");
        return false;
    }

    string str_bdstoken;
    r = FindStrFromString(str_bdstoken, str_response, "msgBdsToken = \"", '"');
    if(!r){
        LOG_MSG("PostTieBaMessage Get BdsToken Failed");
        return false;
    }

    LOG_DEBUG2("TieBaMessage BdsToken = %s", str_bdstoken.c_str());

    //构造发送私信参数
    os << "msgcontent=" << task_tb->content_;
    os << "&msgreceiver=diz59711&refmid=9749090151&vcode=&msgvcode=&bdstoken=" << str_bdstoken;
    os << "&qing_request_source=";
    str_post = os.str();

    r = SendHttpRequestCurl(task_tb, str_url, str_post, str_message_main_page, str_response);
    if(!r){
        LOG_MSG2("PostTieBaMessage Failed, response = %s", str_response.c_str());
        return false;
    }

    string str_errNo;
    r = FindStrFromString(str_errNo, str_response, "errorNo : \"", '"');
    if(r && str_errNo == "0"){
        LOG_DEBUG("PostTieBaMessage Success");
        return true;
    }

    LOG_DEBUG2("PostTieBaMessage fail, errno = %s", str_errNo.c_str());

    return true;
}

TaskWeiBoEngine *TaskWeiBoEngine::instance_ = NULL;
bool TaskWeiBoEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                     string &str_postarg, string &str_referer) {

    bool r = false;
    struct TaskWeiBoPacket *weibo_task = (struct TaskWeiBoPacket *) task;
    if (NULL == weibo_task) {
        LOG_MSG("weibo Task Error");
        return false;
    }

    //从cookie中获取自己的用户id
    string myuid;
    r = FindStrFromString(myuid, task->cookie_, "myuid=", ';');
    if(!r){
        r = FindStrFromString(myuid, task->cookie_, "SID-", '-');
        if(!r){
            LOG_MSG("find myuid failed");
            return false;
        }
    }
    LOG_DEBUG2("FindStrFromString myuid = %s", myuid.c_str());

    //构造post参数
    std::stringstream os;
    os << "act=post&forward=0&isroot=0&location=page_100505_home&module=scommlist&group_source=&_t=0";
    os << "&pdetail=100505" << weibo_task->host_uin_;
    os << "&content=" << weibo_task->content_;
    os << "&mid=" << weibo_task->topic_id_;
    os << "&uid=" << myuid;
    str_postarg = os.str();

    //构造对方微博主页地址
    str_referer = "http://weibo.com/u/" + weibo_task->host_uin_ + "?is_all=1#_0";
    task->pre_url_ = str_referer;

    LOG_DEBUG2("pre_url = %s", str_referer.c_str());

    //清空流，构造提交url
    os.str("");
    os << "http://weibo.com/aj/v6/comment/add?ajwvr=6" << "&__rnd=";
    os << logic::SomeUtils::GetCurrentTimeMs();
    str_url = os.str();

    return true;
}

bool TaskWeiBoEngine::JudgeResultByResponse(string &response,
                                            string &code_num) {

    bool r = false;
    r = FindStrFromString(code_num, response, "{\"code\":\"", '"');
    if (r && code_num == "10000") {
        response = "success";
        code_num = "success";
        return true;
    }

    r = FindStrFromString(code_num, response, "msg:", '。');
    if(r){
        response = code_num;
    }

    return false;
}

void TaskWeiBoEngine::AssembleRequestHeader(struct curl_slist* headers){
    headers = curl_slist_append(headers, "Host: weibo.com");
}

TaskMopEngine *TaskMopEngine::instance_ = NULL;
bool TaskMopEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                   string &str_postarg, string &str_referer) {

    bool r = false;
    struct TaskMopPacket *task_mop = (struct TaskMopPacket *) task;
    if (NULL == task_mop) {
        return false;
    }

    string str_subId;
    r = FindStrFromString(str_subId, task_mop->pre_url_, "tt.mop.com/", '.');
    if (!r) {
        return false;
    }

    std::stringstream os;

    os << "subId=" << str_subId;
    os << "&replys=" << task_mop->content_;
    os << "&pCatId=" << task_mop->pCatId_;
    os << "&catalogId=" << task_mop->catalogId_;
    os << "&fmtoken=" << task_mop->fmtoken_;
    os << "&currformid=" << task_mop->currformid_;
    os << "&date=" << logic::SomeUtils::GetCurrentTimeMs();
    os << "&niming=0";

    str_postarg = os.str();
    str_url = "http://tt.mop.com/reply/add/ajax";
    str_referer = task_mop->pre_url_;

    return true;
}

bool TaskMopEngine::JudgeResultByResponse(string &response, string &code_num) {

    FindStrFromString(code_num, response, "isSuccess\":", ',');
    if (code_num != "true") {
        return false;
    }

    return true;
}

TaskDouBanEngine *TaskDouBanEngine::instance_ = NULL;
bool TaskDouBanEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                      string &str_postarg,
                                      string &str_referer) {

    struct TaskDouBanPacket *douban_task = (struct TaskDouBanPacket *) task;
    if (NULL == douban_task) {
        return false;
    }

    bool r = false;
    string str_ck;
    r = FindStrFromString(str_ck, douban_task->cookie_, "ck=\"", '"');
    if (!r) {
        LOG_MSG("DouBan Task Find PostArg Ck Failed");
        return false;
    }

    std::stringstream os;
    os << "ck=" << str_ck;
    os << "&rv_comment=" << douban_task->content_;
    os << "&start=0";

    str_postarg = os.str();
    str_url = douban_task->pre_url_ + "add_comment";
    str_referer = douban_task->pre_url_;

    return true;
}

bool TaskDouBanEngine::JudgeResultByResponse(string &response,
                                             string &code_num) {

    if (response.find("页面不存在") == string::npos) {
        return false;
    }

    return true;
}

TaskTaoGuBaEngine *TaskTaoGuBaEngine::instance_ = NULL;
bool TaskTaoGuBaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                       string &str_postarg,
                                       string &str_referer) {

    struct TaskTaoGuBaPacket *task_taoguba = (struct TaskTaoGuBaPacket *) task;
    if (NULL == task_taoguba) {
        return false;
    }

    std::stringstream os;

    //固定参数
    os << TAOGUBA_FIXED_PARAM;

    os << "&subject=" << task_taoguba->subject_;
    os << "&topicID=" << task_taoguba->topicID_;
    os << "&body=" << task_taoguba->content_;

    str_postarg = os.str();
    str_referer = "http://www.taoguba.com.cn/Article/" + task_taoguba->topicID_
            + "/1";
    task_taoguba->pre_url_ = str_referer;
    str_url = TAOGUBA_FIXED_POST_REPLY_URL;

    return true;
}

void TaskTaoGuBaEngine::AssembleRequestHeader(struct curl_slist* headers) {

    headers = curl_slist_append(headers, "Host: www.taoguba.com.cn");
}

bool TaskTaoGuBaEngine::JudgeResultByResponse(string &response,
                                              string &code_num) {

    if (string::npos != response.find("今天发布数量已超过上限，请明天再发，谢谢")) {
        response = "今天发布数量已超过上限，请明天再发，谢谢";
        code_num = response;
        return false;
    }

    if (string::npos != response.find("抱歉，您涉嫌恶意行为，暂限制此操作！")) {
        response = "抱歉，您涉嫌恶意行为，暂限制此操作！";
        code_num = response;
        return false;
    }

    if (string::npos != response.find("错误页面_淘股吧")) {
        response = "错误页面_淘股吧";
        code_num = response;
        return false;
    }

    if (string::npos != response.find("该笔名被永封过,无法登录")) {
        response = "该笔名被永封过,无法登录";
        code_num = response;
        return false;
    }

    code_num = "回复成功";

    return true;
}

TaskXueQiuEngine *TaskXueQiuEngine::instance_ = NULL;
bool TaskXueQiuEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                      string &str_postarg,
                                      string &str_referer) {

    struct TaskXueQiuPacket *task_xueqiu = (struct TaskXueQiuPacket *) task;
    if (NULL == task_xueqiu) {
        return false;
    }

    if (task_xueqiu->pre_url_.empty()) {
        LOG_MSG("task_xueqiu pre_url empty");
        return false;
    }

    bool r = false;
    string str_session_token;
    r = GetSessionToken(task_xueqiu, str_session_token);
    if (!r) {
        LOG_MSG("GetSessionToken Fail");
        return false;
    }

    std::stringstream os;
    if (task_xueqiu->topic_id_.empty()) {

        string str_topic_id;
        r = GetCurrReplyId(task_xueqiu->pre_url_, str_topic_id);
        if (!r) {
            LOG_MSG2("GetXueQiuTopicId Failed, pre_url = %s", task_xueqiu->pre_url_.c_str());
            return false;
        }

        //组装post参数
        os << "id=" << str_topic_id;
        os << "&comment=" << task_xueqiu->content_;
        os << "&forward=&session_token=" << str_session_token;

        str_url = XUEQIU_FIXED_POST_REPLY_URL;

    } else {

        os << "url=%2Fstatuses%2Freply.json";
        os << "&data%5Bid%5D=" << task_xueqiu->topic_id_;
        os << "&data%5Bcomment%5D=" << task_xueqiu->content_;
        os << "&data%5B_%5D=" << logic::SomeUtils::GetCurrentTimeMs();
        os << "&data%5Bforward%5D=0";
        os << "&session_token=" << str_session_token;

        str_url = XUEQIU_FIXED_POST_REPLY_URSER_URL;
    }

    str_postarg = os.str();
    str_referer = task_xueqiu->pre_url_;

    return true;
}

void TaskXueQiuEngine::AssembleRequestHeader(struct curl_slist* headers) {
    headers = curl_slist_append(headers, "Host: xueqiu.com");
}

bool TaskXueQiuEngine::JudgeResultByResponse(string &response,
                                             string &code_num) {

    bool r = false;
    r = FindStrFromString(code_num, response, "error_description\":\"", '"');
    if (r) {
        return false;
    }

    code_num = "回复成功";
    response = code_num;

    return true;
}

bool TaskXueQiuEngine::GetSessionToken(struct TaskXueQiuPacket *task,
                                       string &str_session_token) {

    bool r = false;

    //发送Get请求获取 session_token
    string post_arg = "";
    string str_response;
    string str_get_sesson_token_url = XUEQIU_FIXED_GET_SESSION_TOKEN_URL;
    r = SendHttpRequestCurl(task, str_get_sesson_token_url, post_arg,
                            task->pre_url_, str_response);
    if (!r) {
        //TUDO 再获取一次token
        r = SendHttpRequestCurl(task, str_get_sesson_token_url, post_arg,
                                    task->pre_url_, str_response);
        if(!r){
            LOG_MSG("GetXueQiuSessionToken Failed");
            return false;
        }
    }

    //获取session_token
    r = FindStrFromString(str_session_token, str_response, "token\":\"", '"');
    if (!r) {
        LOG_MSG2("ParseXueQiuSessionToken Failed, response = %s", str_response.c_str());
        return false;
    }

    return true;
}

bool TaskXueQiuEngine::GetCurrReplyId(const string &url, string &topic_id) {

    bool r = false;
    if (url.empty()) {
        LOG_MSG("ParseXueQiuReplyId Failed, Url Empty");
        return false;
    }

    std::stringstream os;
    int url_size = url.size();
    for (int32 i = url_size - 1; i >= 0; --i) {
        if (url[i] == '/') {
            break;
        }
        os << url[i];
    }

    string reverse_id = os.str();
    os.str("");
    for (int32 i = reverse_id.size() - 1; i >= 0; --i) {
        os << reverse_id[i];
    }

    topic_id = os.str();

    return true;
}

TaskIGuBaEngine *TaskIGuBaEngine::instance_ = NULL;
bool TaskIGuBaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                     string &str_postarg, string &str_referer) {

    struct TaskIGuBaPacket *task_iguba = (struct TaskIGuBaPacket *) task;
    if (NULL == task_iguba) {
        return false;
    }

    bool r = false;

    //获取topic_id
    string str_topic_id;
    r = FindStrFromString(str_topic_id, task_iguba->pre_url_, ",", '.');
    if (!r) {
        LOG_MSG2("Parse TopicId Failed, pre_url_ = %s", task_iguba->pre_url_.c_str());
        return false;
    }

    long current_time = logic::SomeUtils::GetCurrentTimeMs();

    std::stringstream os;
    os << "http://iguba.eastmoney.com/interf/reply.aspx?callback=jQuery183039066723543111336_";
    os << current_time;
    os << "&action=reply&id=" << str_topic_id;
    os << "&huifuid=";
    os << "&text=" << task_iguba->content_;
    os << "&yzm=&yzm_id=&t_type=1&_=";
    os << current_time;

    str_url = os.str();
    str_referer = task_iguba->pre_url_;
    str_postarg = "";

    return true;
}

void TaskIGuBaEngine::AssembleRequestHeader(struct curl_slist *headers) {
    headers = curl_slist_append(headers, "Host: iguba.eastmoney.com");
}

bool TaskIGuBaEngine::JudgeResultByResponse(string &response,
                                            string &code_num) {
    bool r = false;
    r = FindStrFromString(code_num, response, "me\":\"", '""');
    if (r && code_num == "评论成功") {
        return true;
    }

    return false;
}

TaskTongHuaShunEngine *TaskTongHuaShunEngine::instance_ = NULL;
void TaskTongHuaShunEngine::AssembleRequestHeader(struct curl_slist *headers){
    headers = curl_slist_append(headers, "Host: t.10jqka.com.cn");
}

string TaskTongHuaShunEngine::GetUTime(){
    time_t current_time = time(NULL);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%m-%d+%H:%M", localtime(&current_time));
    std::string time(tmp);
    return time;
}

bool TaskTongHuaShunEngine::GetPid(struct TaskTongHuaShunPacket &task_ths, string &str_pid){

    //获取帖子 id
    bool r = false;
    string str_pageid;
    r = FindStrFromString(str_pageid, task_ths.pre_url_, "/c", '.');
    if(!r){
        LOG_MSG("TongHuaShun Parse Url PageId fail");
        return false;
    }

    string str_postarg;
    string str_response;
    string str_url = TONGHUASHUN_GETPID_URL + str_pageid;
    r = SendHttpRequestCurl(&task_ths, str_url, str_postarg, task_ths.pre_url_, str_response);
    if(!r){
        r = SendHttpRequestCurl(&task_ths, str_url, str_postarg, task_ths.pre_url_, str_response);
        if(!r){
            LOG_MSG("TaskTongHuaShun Init Pid Fail");
            return false;
        }
    }

    //判断请求结果
    string str_errcode;
    r = FindStrFromString(str_errcode, str_response, "errorCode\":", ',');
    if(!r && str_errcode != "0"){
        LOG_MSG2("TaskTongHuaShun Init Pid Fail, errcode = %s", str_errcode.c_str());
        return false;
    }

    //获取 pid
    r = FindStrFromString(str_pid, str_response, "result\":\"", '"');
    if(!r){
        LOG_MSG("TaskTongHuaShun Parse Pid Fail");
        return false;
    }

    LOG_DEBUG2("TongHuaShun pid = %s", str_pid.c_str());

    return true;
}

bool TaskTongHuaShunEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
                                string &str_postarg, string &str_referer){

    struct TaskTongHuaShunPacket *task_ths = (struct TaskTongHuaShunPacket *)task;
    if(NULL == task_ths){
        return false;
    }

    bool r = false;
    string str_pid;
    r = GetPid(*task_ths, str_pid);
    if(!r){
        LOG_MSG("GetTongHuaShun Pid Fail");
        return false;
    }

    //获取avatar
    string str_avatar;
    r = FindStrFromString(str_avatar, task_ths->cookie_, "avatar=", ';');
    if(!r){
        LOG_MSG("TongHuaShun Avatar Parse Fail");
        return false;
    }

    //获取user_id
    string str_userid;
    r = FindStrFromString(str_userid, task_ths->cookie_, "userid=", ';');
    if(!r){
        LOG_MSG("TongHuaShun Parse userid fail");
        return false;
    }

    //获取username
    string str_uname;
    r = FindStrFromString(str_uname, task_ths->cookie_, "uname=", ';');
    if(!r){
        LOG_MSG("TongHuaShun Parse username fail");
        return false;
    }

    //组装Get
    time_t curr_ms = logic::SomeUtils::GetCurrentTimeMs();
    std::stringstream os;
    os << TONGHUASHUN_FIXED_GET_URL;
    os << curr_ms;
    os << "&rid=0&content=";
    os << task_ths->content_;
    os << "&userid=" << str_userid;
    os << "&uname=" << str_uname;
    os << "&avatar=" << str_avatar;
    os << "&utime=" << GetUTime();
    os << "&to=" << str_userid;
    os << "&pid=" << str_pid;
    os << "&_=" << curr_ms;

    str_url = os.str();
    str_referer = task_ths->pre_url_;
    str_postarg = "";

    return true;
}

bool TaskTongHuaShunEngine::JudgeResultByResponse(string &response, string &code_num){

    LOG_DEBUG2("TaskTongHuaShun Response = %s", response.c_str());

    bool r = false;
    string str_errMsg;
    FindStrFromString(str_errMsg, response, "errorMsg\":\"", '"');

    r = FindStrFromString(code_num, response, "errorCode\":", ',');
    if(!r && code_num != "0"){
        code_num = str_errMsg;
        return false;
    }

    return true;
}

} /* namespace base_logic */
