/*
 * auto_task.cc
 *
 *  Created on: 2016年4月27日
 *      Author: Harvey
 */

#include "task_engine.h"

using std::string;

namespace base_logic {

bool TaskEngine::StartTaskWork(struct TaskHead *task, string &str_response){

    if(NULL == task){
        return false;
    }

    bool r = false;

    //获取伪造的ip
    string forgery_ip;
    FindStrFromString(forgery_ip, task->forge_ip_, "", ':');
    task->forge_ip_ = forgery_ip;

    //构造提交参数, url, referer等
    string str_url, str_post, str_referer;
    r = this->HandlerPostArg(task, str_url, str_post, str_referer);
    if(!r){
        LOG_MSG2("task_type = %d, task_id = %d, HandlerPostArg Failed",
                task->task_type_, task->task_id_);
        return false;
    }

    //提交curl
    r = SendHttpRequestCurl(task, str_url, str_post, str_referer, str_response);
    if(!r){
        LOG_MSG2("task_type = %d, task_id = %d, SendHttpRequestCurl Failed",
                task->task_type_, task->task_id_);
        return false;
    }

    //判断执行结果
    r = this->JudgeResultByResponse(str_response);
    if(!r){
        LOG_MSG2("task_type = %d, task_id = %d, JudgeResultByResponse Failed",
                task->task_type_, task->task_id_);
        return false;
    }

    return true;
}

bool TaskEngine::SendHttpRequestCurl(struct TaskHead *task, string &url,
        string &str_postarg, string &str_referer, string &str_response) {

    CURL* curl = NULL;
    struct curl_slist* headers = NULL;
    CURLcode res;
    curl = curl_easy_init();
    if (curl != NULL) {

        //如果UA为空，使用默认的UA
        if(task->forge_ua_.empty()){
            task->forge_ua_ = "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:45.0) Gecko/20100101 Firefox/45.0";
        }

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); //设定为不验证证书和host
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //跳转  301  302
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, task->forge_ua_.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5); // 接收超时时间
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);

        if (!str_postarg.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, true);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_postarg.c_str());
            LOG_MSG2("post_data = %s", str_postarg.c_str());
        }

        headers = curl_slist_append(headers, "Cache-Control: max-age=0");
        headers = curl_slist_append(headers, "Accept-Charset: uft-8");
        headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
        headers = curl_slist_append(headers, "Connection: keep-alive");

        //cookie
        task->cookie_ = "Cookie: " + task->cookie_;
        headers = curl_slist_append(headers, task->cookie_.c_str());

        //伪造UA
        task->forge_ua_ = "User-Agent: " + task->forge_ua_;
        headers = curl_slist_append(headers, task->forge_ua_.c_str());

        //伪造ip
        if(!task->forge_ip_.empty()){
            LOG_MSG2("fore_ip = %s", task->forge_ip_.c_str());
            task->forge_ip_ = "REMOTE_ADDR: " + task->forge_ip_ + ", X-Forwarded-For: " + task->forge_ip_;
            headers = curl_slist_append(headers, task->forge_ip_.c_str());
        }

        //请求来源
        str_referer = "Referer: " + str_referer;
        headers = curl_slist_append(headers, str_referer.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&(str_response));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, TaskEngine::ReadResponse);

        //如果超时，再提交一次
        res = curl_easy_perform(curl);
        if(CURLcode::CURLE_OPERATION_TIMEDOUT == res){
            res = curl_easy_perform(curl);
        }

        LOG_MSG2("url = %s, res = %d, error = %s", url.c_str(), res, curl_easy_strerror(res));
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

bool TaskEngine::FindStrFromString(string &find, const string &src,
        const string start, const char end){

    std::size_t ret = 0;
    if(!start.empty()){
        //检查要查找的字符串中有没有起始字符串
        ret = src.find(start);
        if(ret == string::npos){
            return false;
        }
    }

    //获取查找字符串之后的字符串
    ret = ret + start.size();
    string end_str = src.substr(ret);

    //如果要找到末尾，直接返回
    if('\0' == end){
        find = end_str;
        return true;
    }

    //判断剩余字符串有没有结束字符
    ret = end_str.find(end);
    if(ret == string::npos){
        return false;
    }

    std::stringstream os;
    int end_total_size = end_str.size();
    const char *c_src = end_str.c_str();
    for(int i = 0; i < end_total_size; ++i){
        if(c_src[i] == end){
            break;
        }

        os << c_src[i];
    }

    find = os.str();

    return true;
}

TaskQQEngine *TaskQQEngine::instance_ = NULL;
bool TaskQQEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
        string &str_postarg, string &str_referer){

    bool r = false;
    struct TaskQQPacket *qq_task = (struct TaskQQPacket *)task;
    if(NULL == qq_task){
        return false;
    }

    //获取自己的QQ号
    string my_uin = "";
    r = FindStrFromString(my_uin, qq_task->cookie_, "uin=o0", ';');
    if(!r){
        LOG_MSG("QQZone Task Find Uin Failed");
        return false;
    }

    //获取g_tk
    string g_tk;
    r = GetQQGtkByCookie(g_tk, qq_task->cookie_);
    if(!r){
        return false;
    }

    //对方的QQ号
    char temp[sizeof(int64)];
    sprintf(temp, "%d", qq_task->host_uin_);
    string str_hostUin(temp);

    //组装提交post参数
    std::stringstream os;
    if(qq_task->topic_id_.empty()){

        //QQ 留言板固定参数
        os << "qzreferrer=http%3A%2F%2Fctc.qzs.qq.com%2Fqzone%2Fmsgboard%2Fmsgbcanvas.html%23page%3D1";
        os << "&format=fs&inCharset=utf-8&outCharset=utf-8&iNotice=1&ref=qzone&json=1";

        str_url = "http://m.qzone.qq.com/cgi-bin/new/add_msgb?g_tk=" + g_tk;
        str_referer = "http://ctc.qzs.qq.com/qzone/msgboard/msgbcanvas.html";

    }else{

        //QQ 评论说说固定参数
        os << "qzreferrer=http%3A%2F%2Fuser.qzone.qq.com%2F";
        os << my_uin;
        os << "&feedsType=100&inCharset=utf-8&outCharset=utf-8&plat=qzone&source=ic&isSignIn=&platformid=52";
        os << "&format=fs&ref=feeds&richval=&richtype=&private=0&paramstr=2";
        os << "&topicId=" << qq_task->topic_id_;

        str_url = "http://taotao.qzone.qq.com/cgi-bin/emotion_cgi_re_feeds?g_tk=" + g_tk;
        str_referer = "http://user.qzone.qq.com/" + str_hostUin;
    }

    os << "&hostUin=" << qq_task->host_uin_;
    os << "&uin=" << my_uin;
    os << "&content=" << qq_task->content_;

    str_postarg = os.str();
}

bool TaskQQEngine::JudgeResultByResponse(const string response){

    string code_num;
    FindStrFromString(code_num, response, "\"code\":", ',');
    if(code_num != "0"){
        return false;
    }

    return true;
}

bool TaskQQEngine::GetQQGtkByCookie(string &str_gtk, const string cookie){

    string p_skey;
    bool r = FindStrFromString(p_skey, cookie, "p_skey=", ';');
    if(!r){
        LOG_MSG("GetGtk Failed");
        return false;
    }

    LOG_MSG2("p_skey = %s", p_skey.c_str());

    int hash = 5381;
    int len = p_skey.size();
    for(int i = 0; i < len; ++i){
        hash += (hash << 5) + (int)p_skey[i];
    }

    int64 g_tk =  hash & 0x7fffffff;

    std::stringstream os;
    os << g_tk;
    str_gtk = os.str();

    LOG_MSG2("str_gtk = %s", str_gtk.c_str());

    return true;
}

TaskTianYaEngine *TaskTianYaEngine::instance_ = NULL;
bool TaskTianYaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer){

    bool r = false;
    struct TaskTianYaPacket *tianya_task = (struct TaskTianYaPacket *)task;
    if(NULL == tianya_task){
        return false;
    }

    //http://bbs.tianya.cn/post-water-1727544-1.shtml
    //获取板块名
    string bbs_item;
    r = FindStrFromString(bbs_item, tianya_task->pre_url_, "post-", '-');
    if(!r){
        return false;
    }

    //获取帖子id
    string page_id;
    r = FindStrFromString(page_id, tianya_task->pre_url_, bbs_item + "-", '-');
    if(!r){
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

    //获得post参数
    str_postarg = os.str();

    //提交Url固定
    str_url = "http://bbs.tianya.cn/api?method=bbs.ice.reply";

    //原帖地址
    str_referer = tianya_task->pre_url_;

    return true;
}

bool TaskTianYaEngine::JudgeResultByResponse(const string response){

    string code_num;
    FindStrFromString(code_num, response, "{\"success\":\"", '"');
    if(code_num == "1"){
        return true;
    }

    return false;
}

TaskTieBaEngine *TaskTieBaEngine::instance_ = NULL;
bool TaskTieBaEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer){

    bool r = false;
    struct TaskTieBaPacket *tieba_task = (struct TaskTieBaPacket*)task;
    if(NULL == tieba_task){
        return false;
    }

    //http://tieba.baidu.com/p/4492624045
    //获得tid
    string str_tid;
    r = FindStrFromString(str_tid, tieba_task->pre_url_, "com/p/", '\0');
    if(!r){
        LOG_MSG2("FindStrFromString TieBa Tid = %s failed", str_tid.c_str());
        return false;
    }

    //获取tbs
    string str_tbs;
    r = FindStrFromString(str_tbs, tieba_task->cookie_, "", ';');
    if(!r){
        LOG_MSG2("FindStrFromString TieBa tbs = %s failed", str_tbs.c_str());
        return false;
    }

    //组装post语句
    std::stringstream os;

    if(tieba_task->repost_id_.empty()){
        //固定参数
        os << "ie=utf-8&vcode_md5=&rich_text=1&files=%5B%5D&mouse_pwd=&mouse_pwd_isclick=0&__type__=reply";
        os << "&mouse_pwd_t=" << logic::SomeUtils::GetCurrentTime();
    }else{
        //固定参数
        os << "ie=utf-8&rich_text=1&lp_type=0&lp_sub_type=0&new_vcode=1&tag=11&anonymous=0";
        os << "&quote_id=" << tieba_task->repost_id_;
        os << "&repostid=" << tieba_task->repost_id_;
    }

    // 默认回2楼
    if( tieba_task->floor_num_ < 0){
        tieba_task->floor_num_ = 1;
    }

    tieba_task->floor_num_ += 1;
    os << "&kw=" << tieba_task->kw_;
    os << "&fid=" << tieba_task->fid_;
    os << "&tid=" << str_tid;
    os << "&floor_num=" << tieba_task->floor_num_;
    os << "&" << str_tbs;
    os << "&content=" << tieba_task->content_;

    LOG_MSG2("referer = %s", tieba_task->pre_url_.c_str());
    LOG_MSG2("cookie = %s", tieba_task->cookie_.c_str());
    LOG_MSG2("kw = %s", tieba_task->kw_.c_str());
    LOG_MSG2("fid = %s", tieba_task->fid_.c_str());
    LOG_MSG2("tid = %s", str_tid.c_str());
    LOG_MSG2("%s", str_tbs.c_str());

    //post参数
    str_postarg = os.str();

    //固定提交url
    str_url = "http://tieba.baidu.com/f/commit/post/add";

    //原帖地址
    str_referer = tieba_task->pre_url_;

    return true;
}

bool TaskTieBaEngine::JudgeResultByResponse(const string response){

    string code_num;
    FindStrFromString(code_num, response, "{\"no\":", ',');
    if(code_num == "0"){
        FindStrFromString(code_num, response, "err_code\":", ',');
        if(code_num == "0"){
            return true;
        }
    }

    return false;
}

TaskWeiBoEngine *TaskWeiBoEngine::instance_ = NULL;
bool TaskWeiBoEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
        string &str_postarg, string &str_referer){

    struct TaskWeiBoPacket *weibo_task = (struct TaskWeiBoPacket *)task;
    if(NULL == weibo_task){
        LOG_MSG("weibo Task Error");
        return false;
    }

    //从cookie中获取自己的用户id
    string myuid;
    FindStrFromString(myuid, task->cookie_, "myuid=", ';');
    LOG_MSG2("FindStrFromString myuid = %s", myuid.c_str());

    //从cookie中获取wvr的值
    string wvr_num;
    FindStrFromString(wvr_num, task->cookie_, "wvr=", ';');
    LOG_MSG2("FindStrFromString wvr_num = %s", wvr_num.c_str());

    //构造post参数
    std::stringstream os;
    os << "act=post&forward=0&isroot=0&location=page_100505_home&module=scommlist&group_source=&_t=0";
    os << "&content=" << weibo_task->content_;
    os << "&mid=" << weibo_task->topic_id_;
    os << "&uid=" << myuid;
    str_postarg = os.str();

    //构造对方微博主页地址
    str_referer = "http://weibo.com/u/" + weibo_task->host_uin_ + "?is_all=1#_0";

    //清空流，构造提交url
    os.str("");
    os << "http://weibo.com/aj/v6/comment/add?ajwvr=" << wvr_num << "&__rnd=";
    os << logic::SomeUtils::GetCurrentTime();
    str_url = os.str();

    return true;
}

bool TaskWeiBoEngine::JudgeResultByResponse(const string response){

    string code_num;
    FindStrFromString(code_num, response, "{\"code\":\"", '"');
    if(code_num == "10000"){
        return true;
    }

    return false;
}

TaskMopEngine *TaskMopEngine::instance_ = NULL;
bool TaskMopEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
        string &str_postarg, string &str_referer){

    bool r = false;
    struct TaskMopPacket *task_mop = (struct TaskMopPacket *)task;
    if(NULL == task_mop){
        return false;
    }

    string str_subId;
    r = FindStrFromString(str_subId, task_mop->pre_url_, "tt.mop.com/", '.');
    if(!r){
        return false;
    }

    std::stringstream os;

    os << "subId=" << str_subId;
    os << "&replys=" << task_mop->content_;
    os << "&pCatId=" << task_mop->pCatId_;
    os << "&catalogId=" << task_mop->catalogId_;
    os << "&fmtoken=" << task_mop->fmtoken_;
    os << "&currformid=" << task_mop->currformid_;
    os << "&date=" << logic::SomeUtils::GetCurrentTime();
    os << "&niming=0";

    str_postarg = os.str();
    str_url = "http://tt.mop.com/reply/add/ajax";
    str_referer = task_mop->pre_url_;

    return true;
}

bool TaskMopEngine::JudgeResultByResponse(const string response){

    string code_num;
    FindStrFromString(code_num, response, "isSuccess\":", ',');
    if(code_num != "true"){
        return false;
    }

    return true;
}

TaskDouBanEngine *TaskDouBanEngine::instance_ = NULL;
bool TaskDouBanEngine::HandlerPostArg(struct TaskHead *task, string &str_url,
            string &str_postarg, string &str_referer){

    struct TaskDouBanPacket *douban_task = (struct TaskDouBanPacket *)task;
    if(NULL == douban_task){
        return false;
    }

    bool r = false;
    string str_ck;
    r = FindStrFromString(str_ck, douban_task->cookie_, "ck=\"", '"');
    if(!r){
        LOG_MSG("DouBan Task Find PostArg Ck Failed");
        return false;
    }

    std::stringstream os;
    os << "ck=" << str_ck;
    os << "&rv_comment=" << douban_task->content_;
    os << "&start=0";

    str_postarg = os.str();

    //https://movie.douban.com/review/4521203/
    //https://movie.douban.com/review/4521203/add_comment
    str_url = douban_task->pre_url_ + "add_comment";

    str_referer = douban_task->pre_url_;

    return true;
}

bool TaskDouBanEngine::JudgeResultByResponse(const string response){

    if(response.find("页面不存在") == string::npos){
        return false;
    }

    return true;
}

} /* namespace base_logic */
