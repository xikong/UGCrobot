/*****************************************************
 Copyrignt: 

 Licens: Apache 2.0

 Author: Vander

 Data: 2015-11-14

 Function:  提供两个接口： 
 void SendListInfo(list<CrawlerInfo> ): 从客户端发送list<>信息至ativeMQ服务器

 list<CrawlerInfo> RecvListInfo(int size): 从服务器取指定数量的消息返回客户端，如
 果取的数量大于服务器上队列剩余数量，那么则取完存在的消息，等待waitMillis(默认设置2000毫秒)
 后，如果没有新消息可以取，退出线程。
 *****************************************************/

#include "activemq_client.h"

namespace activemq_logic {

using namespace activemq::core;
using namespace decaf::util::concurrent;
using namespace decaf::util;
using namespace decaf::lang;
using namespace cms;

CrawlerInfoProducer::~CrawlerInfoProducer() {
  cleanup();
}

void CrawlerInfoProducer::close() {
  this->cleanup();
}

void CrawlerInfoProducer::set_crawler_info(CrawlerInfo* tmp) {
  this->crawler_info->task_id = tmp->task_id;
  this->crawler_info->attr_id = tmp->attr_id;
  this->crawler_info->max_depth = tmp->max_depth;
  this->crawler_info->cur_depth = tmp->cur_depth;
  this->crawler_info->URL = tmp->URL;
}

void CrawlerInfoProducer::run() {
  try {
    std::auto_ptr<ConnectionFactory> connectionFactory(
        ConnectionFactory::createCMSConnectionFactory(broker_URI));

    connection = connectionFactory->createConnection();
    connection->start();

    if (this->session_transacted) {
      session = connection->createSession(Session::SESSION_TRANSACTED);
    } else {
      session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
    }

    if (use_topic) {
      destination = session->createTopic("CrawlerInfoSet1201");
    } else {
      destination = session->createQueue("CrawlerInfoSet1201");
    }

    producer = session->createProducer(destination);
    producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);

    // 创建线程ID（测试用）
    std::string threadIdStr = Long::toString(Thread::currentThread()->getId());

    // 创建信息
    std::string text = (std::string) "Test Message! from thread " + threadIdStr;

    for (int ix = 0; ix < MessageNum; ++ix) {
      std::auto_ptr<TextMessage> message(session->createTextMessage(text));

      //设置属性，可将各信息分布加入属性设置
      message->setLongProperty("task_id", this->crawler_info->task_id);
      message->setIntProperty("attr_id", this->crawler_info->attr_id);
      message->setByteProperty("max_depth", this->crawler_info->max_depth);
      message->setByteProperty("cur_depth", this->crawler_info->cur_depth);
      message->setStringProperty("URL", this->crawler_info->URL);
      printf("Sent message #%d from thread %s\n", ix + 1, threadIdStr.c_str());
      producer->send(message.get());
    }
  } catch (CMSException& e) {
    e.printStackTrace();
  }
}

void CrawlerInfoProducer::cleanup() {
  if (connection != NULL) {
    try {
      connection->close();
    } catch (cms::CMSException& ex) {
      ex.printStackTrace();
    }
  }

  try {
    delete destination;
    destination = NULL;

    delete producer;
    producer = NULL;

    delete session;
    session = NULL;

    delete connection;
    connection = NULL;

    if (crawler_info)
      delete crawler_info;
    crawler_info = NULL;
  } catch (CMSException& e) {
    e.printStackTrace();
  }

}

CrawlerInfoConsumer::~CrawlerInfoConsumer() {
  cleanup();
}

void CrawlerInfoConsumer::close() {
  this->cleanup();
}

void CrawlerInfoConsumer::waitUntilReady() {
  latch.await();
}

void CrawlerInfoConsumer::run() {
  try {
    recv_crawler_info = new CrawlerInfo();

    std::auto_ptr<ConnectionFactory> connectionFactory(
        ConnectionFactory::createCMSConnectionFactory(broker_URI));

    connection = connectionFactory->createConnection();
    connection->start();
    connection->setExceptionListener(this);

    if (this->session_transacted == true) {
      session = connection->createSession(Session::SESSION_TRANSACTED);
    } else {
      session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
    }

    if (use_topic) {
      destination = session->createTopic("CrawlerInfoSet1201");
    } else {
      destination = session->createQueue(
          "CrawlerInfoSet1201?consumer.prefetchSize=1");
    }

    consumer = session->createConsumer(destination);

    consumer->setMessageListener(this);

    std::cout.flush();
    std::cerr.flush();

    // 表示可接受信息
    latch.countDown();

    doneLatch.await(waitMillis);
  } catch (CMSException& e) {

    latch.countDown();
    e.printStackTrace();
  }
}

void CrawlerInfoConsumer::onMessage(const Message* message) {
  static int count = 0;
  try {
    ++count;
    const TextMessage* text_message = dynamic_cast<const TextMessage*>(message);
    std::string text = "";

    if (text_message != NULL) {
      text = text_message->getText();

      recv_crawler_info->task_id = text_message->getLongProperty("task_id");
      recv_crawler_info->attr_id = text_message->getIntProperty("attr_id");
      recv_crawler_info->max_depth = text_message->getByteProperty("max_depth");
      recv_crawler_info->cur_depth = text_message->getByteProperty("cur_depth");
      recv_crawler_info->URL = text_message->getStringProperty("URL");
    } else {
      text = "Not a textmessage!";
    }

    printf("Message #%d Received: %s\n", count, text.c_str());
  } catch (CMSException& e) {
    e.printStackTrace();
  }

  if (this->session_transacted) {
    session->commit();
  }

  doneLatch.countDown();
}

void CrawlerInfoConsumer::onException(const CMSException& ex AMQCPP_UNUSED) {
  printf("CMS Exception occurred.  Shutting down client.\n");
  ex.printStackTrace();
  exit(1);
}

void CrawlerInfoConsumer::cleanup() {
  if (connection != NULL) {
    try {
      connection->close();
    } catch (cms::CMSException& ex) {
      ex.printStackTrace();
    }
  }

  try {
    delete destination;
    destination = NULL;

    delete consumer;
    consumer = NULL;

    delete session;
    session = NULL;

    delete connection;
    connection = NULL;

    if (recv_crawler_info)
      delete recv_crawler_info;
    recv_crawler_info = NULL;
  } catch (CMSException& e) {
    e.printStackTrace();
  }
}

void SendSingleInfo(CrawlerInfo* sent_crawler_info) {
  activemq::library::ActiveMQCPP::initializeLibrary();
  {
    std::string broker_URI = "failover:(tcp://192.168.0.2:61616"
    //"?wireFormat=openwire"
    //"&transport.useInactivityMonitor=false"
    //"&connection.alwaysSyncSend=true"
    //"&connection.useAsyncSend=true"
    //"?transport.commandTracingEnabled=true"
    //"&transport.tcpTracingEnabled=true"
    //"&wireFormat.tightEncodingEnabled=true"
            ")";

    bool use_topic = false;
    bool session_transacted = false;
    int numMessages = 1;

    CrawlerInfoProducer producer(broker_URI, numMessages, use_topic,
                                 session_transacted);
    producer.set_crawler_info(sent_crawler_info);

    Thread producerThread(&producer);
    producerThread.start();
    producerThread.join();
    producer.close();
  }
  activemq::library::ActiveMQCPP::shutdownLibrary();
}

CrawlerInfo RecvSingleInfo() {
  CrawlerInfo tmp;
  activemq::library::ActiveMQCPP::initializeLibrary();
  {
    std::string broker_URI = "failover:(tcp://192.168.0.2:61616"
    //"?wireFormat=openwire"
    //"&transport.useInactivityMonitor=false"
    //"&connection.alwaysSyncSend=true"
    //"&connection.useAsyncSend=true"
    //"?transport.commandTracingEnabled=true"
    //"&transport.tcpTracingEnabled=true"
    //"&wireFormat.tightEncodingEnabled=true"
            ")";

    bool use_topic = false;
    bool session_transacted = false;
    int numMessages = 1;

    CrawlerInfoConsumer consumer(broker_URI, numMessages, use_topic,
                                 session_transacted);

    Thread consumerThread(&consumer);
    consumerThread.start();

    consumer.waitUntilReady();
    consumerThread.join();

    tmp = *(consumer.recv_crawler_info);
    consumer.close();
  }
  activemq::library::ActiveMQCPP::shutdownLibrary();
  return tmp;
}

void SendListInfo(std::list<CrawlerInfo*> list_info) {
  if (!list_info.empty()) {
    std::list<CrawlerInfo*>::iterator itr;
    CrawlerInfo* tmp_info;
    for (itr = list_info.begin(); itr != list_info.end(); ++itr) {
      tmp_info = *itr;
      SendSingleInfo(tmp_info);
    }
  }
}

std::list<CrawlerInfo> RecvListInfo(int size) {
  std::list<CrawlerInfo> tmp_list;
  CrawlerInfo tmp;
  int i;
  for (i = 0; i < size; ++i) {
    tmp = RecvSingleInfo();
    tmp_list.push_back(tmp);
  }
  return tmp_list;
}
}  // activemq_logic
