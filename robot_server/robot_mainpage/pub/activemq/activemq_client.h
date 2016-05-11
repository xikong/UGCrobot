#ifndef _MAIN_H_
#define _MAIN_H_





#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <decaf/lang/Integer.h>
#include <decaf/lang/Long.h>
#include <decaf/lang/System.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <list>


namespace activemq {
using namespace activemq::core;
using namespace decaf::util::concurrent;
using namespace decaf::util;
using namespace decaf::lang;
using namespace cms;
using namespace std;


//假设传输的消息格式
typedef struct _CrawlerInfo {
	int taskId;
	std::string URL;
	int maxDepth;
	int curDepth;
}CrawlerInfo;


//定义传输消息对象--用于包装消息
class CrawlerInfoProducer : public Runnable {
private:

    Connection* connection;
    Session* session;
    Destination* destination;
    MessageProducer* producer;
    int numMessages;
    bool useTopic;
    bool sessionTransacted;
    std::string brokerURI; 
    CrawlerInfo* crawlerInfo;

private:

    CrawlerInfoProducer(const CrawlerInfoProducer&);
    CrawlerInfoProducer& operator=(const CrawlerInfoProducer&);

public:
    CrawlerInfoProducer(const std::string& brokerURI, int numMessages, bool useTopic = false, bool sessionTransacted = false) :
        connection(NULL),
        session(NULL),
        destination(NULL),
        producer(NULL),
        numMessages(numMessages),
        useTopic(useTopic),		
        sessionTransacted(sessionTransacted),
        brokerURI(brokerURI) {
	crawlerInfo = new CrawlerInfo();
    }

    virtual ~CrawlerInfoProducer();

    void close();

    void setCrawlerInfo(CrawlerInfo* tmp);

    virtual void run();

private:
	//析构
    void cleanup();
};


//创建消费者，用于接收服务端消息
class CrawlerInfoConsumer : public ExceptionListener,
							public MessageListener,
							public Runnable {

private:

    CountDownLatch latch;
    CountDownLatch doneLatch;
    Connection* connection;
    Session* session;
    Destination* destination;
    MessageConsumer* consumer;
    long waitMillis;
    bool useTopic;
    bool sessionTransacted;
    std::string brokerURI;

private:

    CrawlerInfoConsumer(const CrawlerInfoConsumer&);
    CrawlerInfoConsumer& operator=(const CrawlerInfoConsumer&);

public:
    CrawlerInfo* crawlerInfoReceiver;
    CrawlerInfoConsumer(const std::string& brokerURI, int numMessages, bool useTopic = false, bool sessionTransacted = false, int waitMillis = 2000) :
        latch(1),
        doneLatch(numMessages),
        connection(NULL),
        session(NULL),
        destination(NULL),
        consumer(NULL),
        waitMillis(waitMillis),
        useTopic(useTopic),
        sessionTransacted(sessionTransacted),
        brokerURI(brokerURI) {
    }

    virtual ~CrawlerInfoConsumer();

    void close();

    void waitUntilReady();

    virtual void run();

    // 在该类注册信息监听时调用
    virtual void onMessage(const Message* message);

    // 继承异常监听类，监听异常
    virtual void onException(const CMSException& ex AMQCPP_UNUSED);

private:
    void cleanup();
};



//包装需要发送的信息，定义实际传输消息逻辑
void SendSingleInfo(CrawlerInfo* infoNeedSent);


//接收服务器消息，定义实际接收逻辑
CrawlerInfo RecvSingleInfo();



//定义队列发送接口
void SendListInfo(list<CrawlerInfo*> listInfo);



//定义接收消息队列接口
list<CrawlerInfo> RecvListInfo(int size);

} //namespace -- activemq

#endif



		
	
