#ifndef ACTIVEMQ_CLIENT_H_
#define ACTIVEMQ_CLIENT_H_

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

namespace activemq_logic {

typedef struct _CrawlerInfo {
  long task_id;
  int attr_id;
  unsigned char max_depth;
  unsigned char cur_depth;
  std::string URL;
} CrawlerInfo;

class CrawlerInfoProducer : public Runnable {
 public:

  CrawlerInfoProducer(const std::string& broker_URI, int MessageNum,
                      bool use_topic = false, bool session_transacted = false)
      : connection(NULL),
        session(NULL),
        destination(NULL),
        producer(NULL),
        MessageNum(MessageNum),
        use_topic(use_topic),
        session_transacted(session_transacted),
        broker_URI(broker_URI) {
    crawler_info = new CrawlerInfo();
  }

 public:
  virtual ~CrawlerInfoProducer();
  void close();
  void set_crawler_info(CrawlerInfo* crawler_info);
  virtual void run();

 private:
  void cleanup();

 private:
  Connection* connection;
  Session* session;
  Destination* destination;
  MessageProducer* producer;
  int MessageNum;
  bool use_topic;
  bool session_transacted;
  std::string broker_URI;
  CrawlerInfo* crawler_info;
};

class CrawlerInfoConsumer : public ExceptionListener, public MessageListener,
    public Runnable {
 public:

  CrawlerInfoConsumer(const std::string& broker_URI, int numMessages,
                      bool use_topic = false, bool session_transacted = false,
                      int waitMillis = 2000)
      : latch(1),
        doneLatch(numMessages),
        connection(NULL),
        session(NULL),
        destination(
        NULL),
        consumer(NULL),
        waitMillis(waitMillis),
        use_topic(use_topic),
        session_transacted(session_transacted),
        broker_URI(broker_URI) {
  }
  virtual ~CrawlerInfoConsumer();

 public:
  void close();
  void waitUntilReady();
  virtual void run();
  virtual void onMessage(const Message* message);virtual void onException(const CMSException& ex AMQCPP_UNUSED);

 private:
  void cleanup();

 private:
  CountDownLatch latch;
  CountDownLatch doneLatch;
  Connection* connection;
  Session* session;
  Destination* destination;
  MessageConsumer* consumer;
  long waitMillis;
  bool use_topic;
  bool session_transacted;
  std::string broker_URI;

 public:
  CrawlerInfo* recv_crawler_info;
};

void SendSingleInfo(CrawlerInfo* sent_crawler_info);

CrawlerInfo RecvSingleInfo();

void SendListInfo(std::list<CrawlerInfo*> list_Info);

std::list<CrawlerInfo> RecvListInfo(int size);
}  //activemq_logic

#endif //ACTIVEMQ_CLIENT_H_

