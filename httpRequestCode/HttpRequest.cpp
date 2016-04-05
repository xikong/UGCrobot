#include "Request.h"

//#define COOKIE
#define RESPONSE
//#define URL

Http* Http::pthis;

static std::unordered_set<std::string> visitedUrl;
static std::unordered_set<std::string> jobUrl;
static std::map<std::string , std::string> postArg;// key host, value post arg
static std::map<std::string , void (Http::*)(std::string&, std::string&, std::string&,\
		std::string&, std::string&, std::string&)> compareValue;// key host 

Http::Http():sockfd(0), maxthreadnum(0), fpwUrl(NULL) {
	signal(SIGPIPE, &signalHandler);
	pthread_mutex_init(&urllock, NULL);
}

Http::Http(int num):fpwUrl(NULL), maxthreadnum(num) {
	signal(SIGPIPE, &signalHandler);
	openFile(fpwUrl, "./text", "w+");
	openFile(fprArg, "./config", "r");
	openFile(test, "./test", "r");

	configInit();
	std::unordered_set<std::string>::iterator itr = jobUrl.begin();
	pthis = this;
	curl_global_init(CURL_GLOBAL_ALL);

	try {
		poolInit(maxthreadnum);
		while (true) {
			if (itr == jobUrl.end() || itr->empty()) {
				sleep(1);
				itr = jobUrl.begin();
				continue;
			}

			char* p = (char*)malloc(itr->size()+1);
			if (p == NULL) {
				std::cout << "continue url=" << *itr << std::endl;
				itr = jobUrl.erase(itr);
				continue;
			}
			memset(p, 0, itr->size()+1);
			memcpy(p, itr->c_str(), itr->size());
			poolAddWorker(workJob, (void*)p);
			std::cout << "加入任务" << std::endl; 
			//throw std::string("exit");				//测试
			pthread_mutex_lock(&(pthis->pthreadpool.queuelock));
			itr = jobUrl.erase(itr);
			pthread_mutex_unlock(&(pthis->pthreadpool.queuelock));
		}
	}
	catch (std::string& ex){
		std::cout << "catch=";
		std::cout << ex << std::endl;
	}
}

Http::~Http() {
	pooldestroy();
	if (fpwUrl != NULL) {
		fclose(fpwUrl);
	}
	if (fprArg != NULL) {
		fclose(fprArg);
		fclose(test);
	}
	curl_global_cleanup();

	rd_kafka_consume_stop(kafka_consumer.rkt_, kafka_consumer.partition_);
	rd_kafka_topic_destroy(kafka_consumer.rkt_);
	rd_kafka_destroy(kafka_consumer.rk_);

	pthread_cancel(kafkaInitId);
	pthread_join(kafkaInitId, NULL);
}

void Http::configInit() {
	if (1 == kafka_consumer.Init(0,"test","localhost:9092",NULL)) {
		std::cout << "init success" << std::endl;
	}
	postArgInit();
	hostInit();

	pthread_create(&kafkaInitId, NULL, KafkaInit, NULL);

}

void Http::hostInit() {
	compareValue["m.qzone.qq.com"] = &Http::qqZoneMessage;
	compareValue["taotao.qzone.qq.com"] = &Http::qqZone;
	compareValue["weibo.com"] = &Http::weiBo;
	compareValue["bbs.tianya.cn"] = &Http::bbsTianYa;
}

void Http::postArgInit() {
	Json::Reader reader;
	Json::Value root;

	char* parg = new char[2048];
	while(true) {
		memset(parg, 0, 2048);
		if (feof(fprArg) != 0) {
			break;
		}

		if (fgets(parg, 2047, fprArg) == NULL) {
			continue;
		}
		if (!reader.parse(parg, root)) {
			std::cout << "config reader.parse error data=" << parg << std::endl;
			continue;
		}

		std::string url = root["url"].asString();
		std::string arg = root["arg"].asString();
		postArg[url] = arg;
	}
	delete[] parg;
}


void* Http::KafkaInit(void* arg) {
/*	std::string data;
	char* parg = new char[2048];
	while (true) {
		data.clear();
		memset(parg, 0, 2048);
		//pthis->kafka_consumer.PullData(data);
		fgets(parg, 2048, stdin);
		data = parg;
		visitedUrl.insert(data);
		jobUrl.insert(data);
	}
*/
		std::string data;
		pthis->readFile(pthis->test, data);
		visitedUrl.insert(data);
		jobUrl.insert(data);
}

int Http::kafka_consumer_::Init(const int partition, const char* topic, const char* brokers, MsgConsume msg_consume) {
	char err_str[512];
	partition_ = partition;
	msg_consume_ = msg_consume;

	rd_kafka_conf_t *conf = rd_kafka_conf_new();
	if (NULL == conf) {
		return -1;
	}

	rd_kafka_conf_set(conf, "batch.num.messages", "100", err_str, sizeof(err_str));
	if (!(rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf, err_str, sizeof(err_str)))) {
		return -1;
	}

	rd_kafka_set_log_level(rk_, 1);
	if (rd_kafka_brokers_add(rk_, brokers) == 0) {
		return -1;
	}

	rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
	rkt_ = rd_kafka_topic_new(rk_, topic, topic_conf);
	if (NULL == rkt_) {
		return -1;
	}

	//RD_KAFKA_OFFSET_BEGINNING，从partition消息队列的开始进行consume；
	//RD_KAFKA_OFFSET_END：从partition中的将要produce的下一条信息开始（忽略即当前所有的消息)
	if (rd_kafka_consume_start(this->rkt_, partition, RD_KAFKA_OFFSET_END) == -1) {
		return -1;
	} 
	return 1;
}

int Http::kafka_consumer_::PullData(std::string& data) {
	rd_kafka_message_t *rkmessage;
	rd_kafka_poll(rk_, 0);
	rkmessage = rd_kafka_consume(rkt_, partition_, 10000);
	if (!rkmessage)
		return -2;
	if (rkmessage->err) {
		if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
				rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
			return -1;
		}
		return -3;
	}
	data = std::string((const char*)rkmessage->payload, rkmessage->len);
	rd_kafka_message_destroy(rkmessage);
	return 1;
}

#if 0
void* Http::workJob(void* arg) {// socket 方式  备用
	signal(SIGPIPE, &signalHandler);
	char* url = (char*)arg;
	int sockfd = 0;
	std::string response;
	std::string cookie;
	//pthis->readConfig(pthis->fprconfig, "<cookie>", "<cookie>", cookie);

	pthis->socketInit(sockfd);													// 初始化连接
	pthis->urlConnect(url, sockfd, "http://", 80);  							// 连接url
	pthis->sendHttpGetRequest(url, sockfd, cookie); 							// 发送http GET 请求
	//pthis->sendHttpPostRequest(url, sockfd, pthis->postdatastr, pthis->cookie);  		// 发送http POST 请求
	pthis->readHttpResponse(sockfd, response); 									// 接收http请求
	pthis->findAllUrl(response.c_str(), visitedUrl); 							// 查找所有url
	pthis->findCookie(response.c_str(), cookie); 								// 读cookie  复制到cookie  
	pthis->findUrlValue(response, visitedUrl);									// 判断响应是否是200 是否有错误
	pthis->findNumber(response);												// 
	pthis->findNumber(cookie);				  									// 
	//cookie += '\n';
	//pthis->writeFile(pthis->fpwUrl, pthis->response);   						// 测试写文件
	//	writeFile(fp_wUrl, cookie);   											// 测试写文件
	//	ungzib(response, response);
	close(sockfd);
	delete[] (char*)arg;
}
#endif

void* Http::workJob(void* arg) {
	char* buf = (char*)arg;
	std::string wcookie;
	std::string response;
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(buf, root)) {
		std::cout << "reader.parse error data=" << buf << std::endl;
		return NULL;
	}

	std::string url	 		= root["url"].asString();
	std::string cookie 		= root["cookie"].asString();
	std::string content 	= root["content"].asString();
	std::string topidId 	= root["topidId"].asString();
	std::string hostUin		= root["hostUin"].asString();
	std::string uin			= root["uin"].asString();


	std::string postdatastr;
	std::string uri;
	std::string host;
	pthis->findURI(url.c_str(), host, uri, "http://");
	std::map<std::string , void (Http::*)(std::string&, std::string&, std::string&,\
			std::string&, std::string&, std::string&)>::iterator itcompareValue;
	if ((itcompareValue = compareValue.find(host)) != compareValue.end()) {
		(pthis->*(itcompareValue->second))(postdatastr, content, url, topidId,\
				hostUin, uin);
	}
	else {
		std::cout << "find return" << std::endl;
		return NULL;
	}

	pthis->sendRequstCurl(url, postdatastr, cookie, response);
	pthis->findAllUrl(response.c_str(), visitedUrl); 						// 查找所有url
	pthis->findCookie(response.c_str(), cookie); 							// 读cookie  复制到cookie
	pthis->findUrlValue(response, visitedUrl);								// 判断响应是否是200 是否有错误
	pthis->findNumber(response);											// 
	pthis->findNumber(cookie);
#ifdef COOKIE
	cookie += '\n';
	pthis->writeFile(pthis->fpwUrl, cookie);   								// 测试写文件
#endif

#ifdef RESPONSE
	pthis->writeFile(pthis->fpwUrl, response);   							// 测试写文件
#endif
	//ungzib(response, response);
	free((char*)arg);
	return NULL;
	
}

void Http::qqZoneMessage(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topidId, std::string& hostUin, std::string& uin) {
	std::cout << "qqZoneMessage" << std::endl;
	std::string uri;
	std::string host;
	findURI(url.c_str(), host, uri, "http://");
	std::map<std::string, std::string>::iterator itpostArg;
	if ((itpostArg = postArg.find(host)) != postArg.end()) {
		postdatastr = itpostArg->second;
		postdatastr += "&content=";
		postdatastr += content;
		postdatastr += "&hostUin=";
		postdatastr += hostUin;
		postdatastr += "&uin=";
		postdatastr += uin;
		
	}
	else {
		std::cout << "return qqZonMessage NULL" << std::endl;
		return;
	}
}

void Http::qqZone(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topidId, std::string& hostUin, std::string& uin) {
	std::cout << "qqZone" << std::endl;
	std::string uri;
	std::string host;
	findURI(url.c_str(), host, uri, "http://");
	std::map<std::string, std::string>::iterator itpostArg;
	if ((itpostArg = postArg.find(host)) != postArg.end()) {
		postdatastr = itpostArg->second;
		postdatastr += "&content=";
		postdatastr += content;
		postdatastr += "&topicId=";
		postdatastr += topidId;
		postdatastr += "&hostUin=";
		postdatastr += hostUin;
		postdatastr += "&uin=";
		postdatastr += uin;
	}
	else {
		std::cout << "return qqZone NULL" << std::endl;
		return;
	}
}

void Http::weiBo(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& mid, std::string& hostUin, std::string& uin) {
	std::cout << "weibo" << std::endl;
	std::string uri;
	std::string host;
	findURI(url.c_str(), host, uri, "http://");
	std::map<std::string, std::string>::iterator itpostArg;
	if ((itpostArg = postArg.find(host)) != postArg.end()) {
		postdatastr = itpostArg->second;
		postdatastr += "&content=";
		postdatastr += content;
		postdatastr += "&mid=";
		postdatastr += mid;
	}
	else {
		std::cout << "return weibo NULL" << std::endl;
		return;
	}
}

void Http::bbsTianYa(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topidId, std::string& hostUin, std::string& uin) {
	std::cout << "bbsTianYa" << std::endl;
	std::string uri;
	std::string host;
	findURI(url.c_str(), host, uri, "http://");
	std::map<std::string, std::string>::iterator itpostArg;
	if ((itpostArg = postArg.find(host)) != postArg.end()) {
		postdatastr = itpostArg->second;
		postdatastr += "&params.content=";
		postdatastr += content;
		postdatastr += "&params.artId=";
		postdatastr += topidId;
		postdatastr += "&params.item=";
		postdatastr += hostUin;
		if (!uin.empty()) {
			postdatastr += "&params.replyId=";
			postdatastr += uin;
		}
	}
	else {
		std::cout << "return bbsTianYa NULL" << std::endl;
		return;
	}
}



void Http::findTable(std::string& response, std::string& postdatastr, std::string& url) { // form  有action 和 method 
	if (response.empty()) {
		return;
	}

	std::string action;
	std::string method;

	const char* p = response.c_str();
	const char* begin = NULL;
	const char* end = NULL;
	while (true) {
		if (begin = strstr(p, "<form")) {
			if (end = strstr(begin, "</form>")) {
				end += strlen("</form>");
				std::string buf(begin, end);
				if ((buf.find("action") != std::string::npos) && (buf.find("method") != std::string::npos) && (buf.find("textarea") != std::string::npos) /*&& (buf.find("style") != std::string::npos)*/) {
					//writeFile(fpwUrl, buf);   							// 测试写文件
					std::cout << "form 已找到" << std::endl;
					findHtmlValue(buf, "action=\"", action);
					findHtmlValue(buf, "method=\"", method);
					findHtmlName(buf, postdatastr);
					postdatastr[postdatastr.size()-1] = '\0';
					break;
				}
				p = end;
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}

	if (action.empty()) {
		return;
	}

	if (action.find("http://") != std::string::npos) {
		url = action;
		return;
	}

	if (action[0] != '/' && url[url.size()-1] != '/') {
		url += "/";
	}
	url += action;
}

void Http::findHtmlName(std::string& table, std::string& request) {
	const char* ptable = table.c_str(); 
	std::string value;
	std::string encodeValue;

	const char* begin = NULL;
	const char* end = NULL;
	const char* res = NULL;
	while (true) {
		if (begin = strstr(ptable, "<")) {
			if (end = strstr(begin, ">")) {
				std::string str(begin, end);
				if ((str.size() < 10)) {
					ptable = end;
					continue;
				}
				res = findHtmlValue(str.c_str(), "name=\"", request);
				if (res == NULL) {
					ptable = end;
					continue;
				}
				request += "=";
				value.clear();
				encodeValue.clear();
				if (str.find("textarea") == std::string::npos) {
					findHtmlValue(res, "value=\"", value);
				}
				else {
					value = "写的真好";
				}
				urlEncode(value, encodeValue);
				request += encodeValue;
				request += "&";
			}
			else {
				break;
			}
		}
		else {
			break;
		}
		ptable = end;
	}
}

void Http::findHtmlValue(std::string& table, const char* needle, std::string& value) {
	const char* begin = NULL;
	const char* end = NULL;
	if (begin = strstr(table.c_str(), needle)) {
		begin += strlen(needle);
		if (end = strstr(begin, "\"")) {
			value = std::string(begin, end);
		}
	}
}

const char* Http::findHtmlValue(const char* pstr, const char* needle, std::string& value) {
	const char* begin = NULL;
	const char* end = NULL;
	if (begin = strstr(pstr, needle)) {
		begin += strlen(needle);
		if (end = strstr(begin, "\"")) {
			value += std::string(begin, end);
			return end;
		}
	}
	return NULL;
}

int Http::sendRequstCurl(std::string& url, std::string& postdatastr, std::string& cookie, std::string& response) {
	
	CURL* curl = NULL;
	struct curl_slist* headers = NULL;
	CURLcode res;
	curl = curl_easy_init();
	if (curl != NULL) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); //设定为不验证证书和host
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //跳转  301  302
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:37.0) Gecko/20100101 Firefox/37.0");
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "./cookie.txt");
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "./cookie.txt");
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15); // 接收超时时间
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);

		if (!postdatastr.empty()) {
			curl_easy_setopt(curl, CURLOPT_POST, true);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdatastr.c_str());
			std::cout << "post data= " << postdatastr << std::endl;
		}

		headers = curl_slist_append(headers, cookie.c_str());
		headers = curl_slist_append(headers, "Cache-Control: max-age=0");
		headers = curl_slist_append(headers, "Accept-Charset: uft-8");
		headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:37.0) Gecko/20100101 Firefox/37.0");
		headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
		headers = curl_slist_append(headers, "Connection: keep-alive");
		if (url.find("weibo") != std::string::npos) {
			headers = curl_slist_append(headers, "Referer:  http://weibo.com/u/5840015544/home?wvr=5&sudaref=www.baidu.com"); //哪里链接来的 weibo //tianya
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&(response));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Http::readResponse);
		pthread_mutex_lock(&(pthis->pthreadpool.queuelock));
		res = curl_easy_perform(curl);
		pthread_mutex_unlock(&(pthis->pthreadpool.queuelock));
		//std::cout << "error=" << curl_easy_strerror(res) << std::endl;
		curl_easy_cleanup(curl);
	}
	else {
		return 0;
	}
	
	return 1;
}

size_t Http::readResponse(void* buffer, size_t size, size_t member, void* res) {
	size_t ressize = size*member;
	*((std::string*)res) += std::string((char*)buffer, ressize);
	return ressize;
}

void Http::poolInit(int maxthreadnum) {
	pthread_mutex_init(&urllock, NULL);
	pthread_mutex_init(&pthreadpool.queuelock, NULL);
	pthread_cond_init(&pthreadpool.queueready, NULL);
	pthreadpool.maxthreadnum = maxthreadnum;
	pthreadpool.threadid = new pthread_t[maxthreadnum];
	for (int i = 0; i < maxthreadnum; ++ i) {
		pthread_create(&(pthreadpool.threadid[i]), NULL, threadfunction, NULL);	
	}
}

void* Http::threadfunction(void* arg) {
	while (true) {
		pthread_mutex_lock(&(pthis->pthreadpool.queuelock));
		while (pthis->pthreadpool.curqueuesize == 0 && pthis->pthreadpool.shutdown == 0) {
			pthread_cond_wait(&(pthis->pthreadpool.queueready), &(pthis->pthreadpool.queuelock));
		}

		if (pthis->pthreadpool.shutdown == 1) {
			pthread_mutex_unlock(&(pthis->pthreadpool.queuelock));
			pthread_exit(NULL);
		}

		assert(pthis->pthreadpool.curqueuesize != 0);
		assert(pthis->pthreadpool.queuehead != NULL);

		pthis->pthreadpool.curqueuesize--;
		worker* newworker = pthis->pthreadpool.queuehead;
		pthis->pthreadpool.queuehead = newworker->next;
		pthread_mutex_unlock(&(pthis->pthreadpool.queuelock));
		(*(newworker->process))(newworker->arg);
		delete newworker;
		newworker = NULL;
	}
}

void Http::poolAddWorker(void* (*process)(void* arg), void* arg) {
	worker* newworker = new worker;
	newworker->process = process;
	newworker->arg = arg;

	if (pthreadpool.queuehead == NULL) {
		pthreadpool.queuehead = pthreadpool.queuetail = newworker;
	}
	else {
		pthreadpool.queuetail->next = newworker;
		pthreadpool.queuetail = newworker;
	}

	assert(pthreadpool.queuehead != NULL);
	pthreadpool.curqueuesize++;
	pthread_cond_signal(&pthreadpool.queueready);
	return;
}

void Http::pooldestroy() {
	if (pthreadpool.shutdown == 1) {
		return;
	}
	
	pthreadpool.shutdown = 1;
	pthread_cond_broadcast(&pthreadpool.queueready);

	for (int i = 0; i < maxthreadnum; ++i) {
		pthread_join(pthreadpool.threadid[i], NULL);
	}
	delete[] pthreadpool.threadid;

	worker* head = NULL;
	while(pthreadpool.queuehead) {
		head = pthreadpool.queuehead;
		pthreadpool.queuehead = pthreadpool.queuehead->next;
		delete head;
	}

	pthread_mutex_destroy(&pthreadpool.queuelock);
	pthread_mutex_destroy(&urllock);
	pthread_cond_destroy(&pthreadpool.queueready);
}

int Http::readConfig(FILE* fp, const char* strHead, const char* strTail, std::string& data) {
	if (config.empty()) {
		readFile(fp, config);
	}

	const char* begin = strstr(config.c_str(), strHead);
	if (begin) {
		begin += strlen(strHead) + 1;
		const char* end = strstr(begin, strTail);
		if (end) {
			data = std::string(begin, end - begin);
		}
	}
	else {
		return 0;
	}
	data[data.size() - 1] = '\0';
	return 1;
}

void Http::openFile(FILE*& fp, const char* path, const char* mode) {
	fp = fopen(path, mode);
	if (fp == NULL) {
		throw std::string("fopen error");
	}
}

void Http::readFile(FILE* fp, std::string& data) {
	char str[1024] = {0};
	while (true) {
		if (fread(&str, sizeof(str), 1, fp) != 1) {
			data += std::string(str, sizeof(str));
			break;
		}
		data += std::string(str, sizeof(str));
		memset(str, 0, sizeof(str));
	}
} 

void Http::findUrlValue(std::string& str, std::unordered_set<std::string>& url) {
	if (str.size() < 10) {
		return;
	}

	std::string value(&str[9], '\0', 3);
	int val = atoi(value.c_str());
	switch (val) {
	case 302:
	case 301:
	//	std::cout << "301 302 服务器移动" << std::endl;
		value302(str, url);
		break;
	case 200:
		break;
	case 204:
	//	std::cout << " 204 服务器成功处理了请求，但没有返回任何内容。" << std::endl;
		break;
	case 404:
	//	std::cout << "404 请求资源不存在" << std::endl;
		break;
	default:
	//	std::cout << "default error " << val << std::endl;
		break;
	}
}

void Http::value302(std::string& response, std::unordered_set<std::string>& url) {
	char* str = new char[response.size() + 1];
	memset(str, 0, response.size() + 1);
	memcpy(str, response.c_str(), response.size());
	strlwr(str);

	const char* tagbegin = strstr(str, "location: ");
	if (tagbegin) {
		std::string buf;
		tagbegin += strlen("location: ");
		const char* tagend = strstr(tagbegin, "\r");
		if(tagend) {
			buf = std::string(tagbegin, tagend - tagbegin);
		}
		else {
			tagend = strstr(tagbegin, "\n");
			if(tagend) {
				buf = std::string(tagbegin, tagend - tagbegin);
			}
		}
		addUrl(buf, url);
	}
	delete[] str;
}

void Http::socketInit(int& sockfd) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		std::cout << "socket error" << std::endl;
	}
}

void Http::urlConnect(const char* url, int& sockfd, const char* needle, int port) {
	std::string host;
	std::string uri;

	findURI(url, host, uri, needle);
	for (int i = 0; i < host.size(); ++ i) {
		if ('0' < host[i] && host[i] < '9') {
			std::cout << url << std::endl;
			sockfd = 0;
			return;
		}
	}

	signal(SIGPIPE, &signalHandler);
	struct hostent* phost = NULL;
	if ((phost = gethostbyname(host.c_str())) == NULL) {
		sockfd = 0;
		std::cout << url << std::endl;
		std::cout << "gethostbyname error" << std::endl;
		return;
	}

	char addr[64] = {0};
	struct sockaddr_in clentaddr;
	memset(&clentaddr, 0, sizeof(clentaddr));
	clentaddr.sin_family = AF_INET;
	strcpy(addr, inet_ntoa(*((struct in_addr*)phost->h_addr)));
	clentaddr.sin_addr.s_addr = inet_addr(addr);
	clentaddr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr*)&clentaddr, sizeof(struct sockaddr)) != 0) {
		std::cout << url << std::endl;
		perror("");
		std::cout << "connect error" << std::endl;
	}
	return;
}

void Http::findAllUrl(const char* str, std::unordered_set<std::string>& visitedUrl) {
	if (str == NULL) {
		return;
	}

	const char* tag = "href=\"";
	const char* pos = strstr(str, tag);
	size_t tagsize = strlen(tag);

	while (pos) {
		pos += tagsize;
		if (pos == NULL) {
			std::cout << "NULL" << std::endl;
			break;
		}
		const char* next = strstr(pos, "\"");
		if (next) {
			char* url = new char[next - pos + 1];
			memset(url, 0, next - pos + 1);
			memcpy(url, pos, next - pos);
			strlwr(url);
			std::string surl(url);
			addUrl(surl, visitedUrl);
			delete[] url;
		}
		pos = strstr(pos, tag);
	}
}

void Http::addUrl(std::string& url, std::unordered_set<std::string>& visitedUrl) { 

	if ((url.find("http://") == std::string::npos) || (url.size() < 16) || (url.find("com") == std::string::npos) || url[0] != 'h') {
		return;
	}
	
	if ((url.find(" ") != std::string::npos) || (url.find("\n") != std::string::npos) || (url.find("\r") != std::string::npos)) {
		std::string::iterator it = url.begin();
		for (; it != url.end();) {
			if ((*it == ' ') || (*it == '\n') || (*it == '\r')) {
				it = url.erase(it);
				continue;
			}
			++ it;
		}
	}

	if (strstr(url.c_str() + strlen("http://"), "/") == NULL) {
		url += "/";
	}

	pthread_mutex_lock(&urllock);
	if (visitedUrl.find(url) == visitedUrl.end()) {
		//visitedUrl.insert(url);
		//jobUrl.insert(url);
#ifdef URL
		url += "\n";
		writeFile(fpwUrl, url);
#endif
	}
	pthread_mutex_unlock(&urllock);
}

void Http::strlwr(char* str) {
	if (str == NULL) {
		throw std::string("strlwr NULL ......");
	}

	while (*str++ != '\0') {
		*str = tolower(*str);		
	}
}

void Http::findURI(const char* url, std::string& host, std::string& uri, const char* needle) {
	if ((url = strstr(url, needle)) == NULL) {
		return;
	}

	int urlsize = strlen(url);
	char* url1 = new char[urlsize+2];
	char* purl = url1;
	memset(url1, 0, urlsize+2);
	memcpy(url1, url, urlsize);
	url1 += strlen(needle);
	const char* p = strstr(url1, "/");
	const char* p1 = strstr(url1, "?");
	if (p == NULL) {
		strcat(url1, "/");
		p = strstr(url1, "/");
	}
	if (p1 == NULL) {
		strcat(url1, "?");
		p1 = strstr(url1, "?");
	}

	if (p1 - p < 0) {
		host = std::string(url1, p1 - url1);
		uri = std::string(p1, urlsize - (p1 - url1) - strlen(needle));
	}
	else if (p1 - p > 0) {
		host = std::string(url1, p - url1);
		uri = std::string(p, urlsize - (p - url1) - strlen(needle));
	}
	else {
		delete[] purl;
		std::cout << "url=" << url << std::endl;
		throw std::string("不能区分主机名");
	}
	delete[] purl;
}

void Http::findPath(const char* uri, char* path) {
	const char* p = strstr(uri, "?");
	if (p != NULL) {
		memcpy(path, uri, p - uri);
	}
	else {
		path[0] = '/';
	};
}

void Http::readHttpResponse(int& sockfd, std::string& response) {
	if (sockfd == 0) {
		return;
	}

	response.clear();
	std::string responsebuf;

	struct timeval timeout = {3, 0};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	char* buf = new char[2048];
	memset(buf, '\0', 2048);
	int ret = 1;
	while (ret > 0) {
		ret = recv(sockfd, buf, 2047, 0);
		if (ret > 0) {
			responsebuf += buf;
			memset(buf, 0, 2048);
		}
	}
	delete[] buf;
	response = responsebuf;
}

void Http::writeFile(FILE* fp, std::string& data) {
	if ( fwrite (data.c_str(), data.size(), 1, fp) < 0) {
		std::cout << "fwrite error" << std::endl;
	}
	fflush(fp);
}

void Http::findCookie(const char* cookie, std::string& str) {
	if (cookie == NULL) {
		return;
	}
	str.clear();

	while (true) {  
		const char* cookbegin = strstr(cookie, "Cookie: ");
		if (cookbegin != NULL) {
			const char* cookend = strstr(cookbegin, "\n");
			if (cookend != NULL) {
				cookbegin += strlen("Cookie: ");
				int i = cookend - cookbegin + 1;
				str += std::string(cookbegin, '\0', i);
				cookie = cookend;
			}
		}
		else {
			break;
		}
	}
}

void Http::sendHttpGetRequest(const char* url, int& sockfd, std::string& cookie) {
	if (sockfd == 0) {
		return;
	}

	std::string uri;
	std::string host;
	findURI(url, host, uri, "http://");

	std::string request = "GET " + uri + " HTTP/1.1\r\n";

	request += "Host: " + host + "\r\n";
	request += "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:37.0) Gecko/20100101 Firefox/37.0\r\n";
	request += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
	request += "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n";
	request += "Accept-Encoding: identity\n";
//	request += "Accept-Encoding: gzip\r\n";
	request += "Cookie: " + cookie + "\r\n";
	request += "Connection: keep-alive\r\n";
	request += "Accept-Charset: uft-8\r\n\r\n";

	if (send(sockfd, request.c_str(), request.size(), MSG_NOSIGNAL) < 0 ) {
		std::cout << "GET send error"  << std::endl;
	}
	return;
}

void Http::sendHttpPostRequest(const char* url, int& sockfd, std::string& postdatastr, std::string& cookie) {
	if (sockfd == 0) {
		return;
	}
	char path[128] = {0};
	char length[8] = {0};
	std::string host;
	std::string uri;
	findURI(url, host, uri, "http://");
	findPath(uri.c_str(), path);
	sprintf(length, "%lu", postdatastr.size());

	std::string request = "POST " + std::string(path) + " HTTP/1.1\r\n";
	request += "Host: " + host + "\r\n";
	request += "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:37.0) Gecko/20100101 Firefox/37.0\r\n";
	request += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
	request += "Accept-Language: zh-CN,zh;q=0.8\r\n";
	request += "Connection: keep-alive\r\n";
	request += "Accept-Charset: uft-8\r\n";
	request += "Accept-Encoding: gzip\r\n";
	request += "Content-Type: application/x-www-form-urlencoded\r\n"; 
	request += "Content-Length: " + std::string(length) + "\r\n";
	request += "Cookie: " + cookie + "\r\n\r\n";
	request += postdatastr + "\r\n\r\n";

	if (send(sockfd, request.c_str(), request.size(), 0) < 0 ) {
		std::cout << "POST send error" << std::endl;
	}
	return;
}

void Http::findNumber(const std::string& str) { // 查找号码
	if (str.empty()) {
		return;
	}

	const std::regex pattern("((1[3|4|5|7|8|][0-9]|15[0|3|6|7|8|9]|18[8|9])\\d{8}$)");
	std::match_results<std::string::const_iterator> result;
	bool valid = std::regex_match(str, result, pattern);
	int size = result.size();
	if (valid) {
		for (size_t i = 1; size < i; ++i) {
			std::cout << result[i] << std::endl;
		}
	}
}

void Http::urlEncode(const std::string &in, std::string& out) {
	std::string strTemp = "";
	size_t length = in.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)in[i]) ||
				(in[i] == '-') ||
				(in[i] == '_') ||
				(in[i] == '.') ||
				(in[i] == '~'))
			strTemp += in[i];
		else if (in[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += toHex((unsigned char)in[i] >> 4);
			strTemp += toHex((unsigned char)in[i] % 16);
		}
	}
	out = strTemp;
}

inline char Http::toHex(const char &x){
	return x > 9 ? x + 55: x + 48;
}
