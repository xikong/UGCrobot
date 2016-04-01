#ifndef __REQUEST__H__
#define __REQUEST__H__

#include <jsoncpp/json/json.h>
#include <unordered_set>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <algorithm>
#include <pthread.h>
#include "rdkafka.h"
#include <assert.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <iconv.h>
#include <string>
#include <cstdio>
#include <zlib.h>
#include <regex>
#include <map>

int gzdecompress(Byte *zdata, uLong nzdata, Byte *data, uLong *ndata);
static void ungzib(std::string& compress, std::string& uncompress);
int ungzip(const char* source, int len, char* des);
void signalHandler(int);

typedef void(*MsgConsume)(rd_kafka_message_t *rkmessage, void *opaque);

class Http {
public:
	Http();
	Http(int num);

	~Http();

public:
	void socketInit(int& sockfd);
	void configInit();
	inline void strlwr(char* str);
	void findNumber(const std::string& str);
	void findPath(const char* uri, char* path);
	void readFile(FILE* fp, std::string& cookie);
	inline void writeFile(FILE* fp, std::string& data);
	void openFile(FILE*& fp, const char* path, const char* mode);
	void urlConnect(const char* url, int& sockfd, const char* needle, int port);
	void value302(std::string& response, std::unordered_set<std::string>& url);
	void addUrl(std::string& url, std::unordered_set<std::string>& visitedUrl);
	int readConfig(FILE* fp, const char* strHead, const char* strTail, std::string& data);
	void findURI(const char* url, std::string& host, std::string& uri, const char* needle);

public:
	static void* KafkaInit(void* arg);

private:
	void hostInit();
	void pooldestroy();
	void postArgInit();
	void poolInit(int maxthreadnum);
	inline char toHex(const char &x);
	void findCookie(const char* cookie, std::string& str);
	void urlEncode(const std::string &in, std::string& out);
	void readHttpResponse(int& sockfd, std::string& response);
	void poolAddWorker(void* (*process)(void* arg), void* arg);
	void findHtmlName(std::string& table, std::string& request);
	void findUrlValue(std::string& str, std::unordered_set<std::string>& url);
	void sendHttpGetRequest(const char* url, int& sockfd, std::string& cookie);
	void findAllUrl(const char* str, std::unordered_set<std::string>& visitedUrl);
	void findTable(std::string& response, std::string& postdatstr, std::string& url); // form  有action 和 method 
	void findHtmlValue(std::string& table, const char* needle, std::string& value);
	const char* findHtmlValue(const char* pstr, const char* needle, std::string& value);
	void sendHttpPostRequest(const char* url, int& sockfd, std::string& postDataStr, std::string& cookie);
	int sendRequstCurl(std::string& url, std::string& postdatastr, std::string& cookie, std::string& response);

	void qqZone(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topidId, std::string& hostUin, std::string& uin);
	void qqZoneMessage(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topidId, std::string& hostUin, std::string& uin);
	void weiBo(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& mid, std::string& hostUin, std::string& uin);

	void bbsTianYa(std::string& postdatastr, std::string& content, std::string& url,\
		std::string& topicId, std::string& hostUin, std::string& uin);

	static void* workJob(void* arg);
	static void* threadfunction(void* arg);
	static size_t readResponse(void* buffer, size_t size, size_t member, void* res); 

public:
	std::string url;
	pthread_mutex_t urllock;

public:
	struct worker {
		worker() {memset(this, 0, sizeof(worker));}
		void* (*process) (void* arg);
		void* arg;
		struct worker* next;
	};

	struct pool{
		pool() {memset(this, 0, sizeof(pool));}
		pthread_mutex_t queuelock;
		pthread_cond_t  queueready;
		worker* queuehead;
		worker* queuetail;
		pthread_t* threadid;
		int shutdown;
		int maxthreadnum;
		int curqueuesize;
	};

	class kafka_consumer_
	{
		public:

			int Init(const int in_partition, const char* in_topic, const char* in_brokers, MsgConsume msg_consume);
			int PullData(std::string& out_data);

			kafka_consumer_(){}
			~kafka_consumer_(){}

			int partition_;
			rd_kafka_t *rk_;
			rd_kafka_topic_t *rkt_;
			MsgConsume msg_consume_;
	};

private:
	int sockfd;
	int maxthreadnum;
	size_t postdatasize;
	pthread_t kafkaInitId;
	FILE* fpwUrl;
	FILE* fprArg;
	FILE* test; /////////
	std::string uri;
	std::string host;
	std::string config;
	//std::string postdatastr;
	//std::string response;
	//std::string cookie;
	pool pthreadpool;
	worker pthreadworker;
	kafka_consumer_ kafka_consumer;

private:
	static Http* pthis;
};



class Https :public Http {
public:
	Https();
	~Https();

private:
	void readHttpsResponse(SSL*& ssl, std::string& response);
	void sendHttpsGetRequest(const char* url, SSL*& ssl, std::string& cookie);
	void urlConnectSSL(const char* url, int& sockfd, SSL*& ssl, SSL_CTX*& ctx);


private:
	SSL* ssl;
	SSL_CTX* ctx;
	
};

class HttpSMTP: public Http {
	public:
		HttpSMTP();
		HttpSMTP(const char* host);

		~HttpSMTP();
	
	public:
		std::string base64Encode(const unsigned char* Data,int DataByte);
		std::string base64Decode(const char* Data,int DataByte,int& OutByte);

	private:
		void sendHttpSMTPRequest(int& sockfd, const char* passwd, const char* sendmail, const char* recvmail, const char* data, const char* subject);

	private:
		int sockfd;
		std::string hostid;
};
#endif
