/*****************************************************
Copyrignt: 

Licens: Apache 2.0

Author: Vander

Data: 2015-11-14

Function:  提供两个接口：
		   void SendListInfo(list<CrawlerInfo> ): 从客户端发送list<>信息至ativeMQ服务器

		   list<CrawlerInfo> RecvListInfo(int size): 从服务器取指定数量的消息返回客户端，如
		   果取的数量大于服务器上队列剩余数量，那么则取完存在的消息，等待waitMillis(默认设置5000毫秒)
		   后，如果没有新消息可以取，退出线程。
*****************************************************/

#include "activemq_client.h"

namespace activemq {

CrawlerInfoProducer::~CrawlerInfoProducer() {
	cleanup();
}

void CrawlerInfoProducer::close() {
	cleanup();
}


void CrawlerInfoProducer::setCrawlerInfo(CrawlerInfo* tmp) {
	this->crawlerInfo->curDepth = tmp->curDepth;
	this->crawlerInfo->maxDepth = tmp->maxDepth;
	this->crawlerInfo->taskId = tmp->taskId;
	this->crawlerInfo->URL = tmp->URL;
}

void CrawlerInfoProducer::run() {
        try {
            // 创建连接工厂
            auto_ptr<ConnectionFactory> connectionFactory(
                ConnectionFactory::createCMSConnectionFactory(brokerURI));

            // 创建连接
            connection = connectionFactory->createConnection();
            connection->start();

            // 创建会话（线程）
            if (this->sessionTransacted) {
                session = connection->createSession(Session::SESSION_TRANSACTED);
            } else {
                session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
            }

            // 创建目标信息集
            if (useTopic) {
                destination = session->createTopic("CrawlerInfoSets1114");
            } else {
                destination = session->createQueue("CrawlerInfoSets1114");
            }

            // 会话创建生产者
            producer = session->createProducer(destination);
            producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);

            // 创建线程ID（测试用）
            string threadIdStr = Long::toString(Thread::currentThread()->getId());

            // 创建信息
            string text = (string) "Test Message! from thread " + threadIdStr;

            for (int ix = 0; ix < numMessages; ++ix) {
                std::auto_ptr<TextMessage> message(session->createTextMessage(text));

				//设置属性，可将各信息分布加入属性设置
				message->setIntProperty("curDepth",this->crawlerInfo->curDepth);
				message->setIntProperty("maxDepth",this->crawlerInfo->maxDepth);
				message->setIntProperty("taskId",this->crawlerInfo->taskId);
				message->setStringProperty("URL",this->crawlerInfo->URL);
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

		delete crawlerInfo;
		crawlerInfo = NULL;
	} catch (CMSException& e) {
		e.printStackTrace();
	}
}

CrawlerInfoConsumer::~CrawlerInfoConsumer(){
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
		crawlerInfoReceiver = new CrawlerInfo();
		// 创建连接工厂
		auto_ptr<ConnectionFactory> connectionFactory(
				ConnectionFactory::createCMSConnectionFactory (brokerURI));

		// 创建连接
		connection = connectionFactory->createConnection();
		connection->start();
		connection->setExceptionListener(this);

		// 创建会话（线程）
		if (this->sessionTransacted == true) {
			session = connection->createSession(Session::SESSION_TRANSACTED);
		} else {
			session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
		}

		// 创建目标信息集
		if (useTopic) {
			destination = session->createTopic("CrawlerInfoSets1114");
		} else {
			destination = session->createQueue(
					"CrawlerInfoSets1114?consumer.prefetchSize=1");
		}

		// 创建消费对象
		consumer = session->createConsumer(destination);

		consumer->setMessageListener(this);

		std::cout.flush();
		std::cerr.flush();

		// 表示可接受信息
		latch.countDown();

		// 等待异步消息进入
		doneLatch.await(waitMillis);

	} catch (CMSException& e) {

		latch.countDown();
		e.printStackTrace();
	}
}

void CrawlerInfoConsumer::onMessage(const Message* message) {

	static int count = 0;

	try {
		count++;
		const TextMessage* textMessage =
				dynamic_cast<const TextMessage*>(message);
		string text = "";

		if (textMessage != NULL) {
			text = textMessage->getText();

			//从消息中取出属性（int、long、string）
			crawlerInfoReceiver->curDepth = textMessage->getIntProperty(
					"curDepth");
			crawlerInfoReceiver->URL = textMessage->getStringProperty("URL");
			crawlerInfoReceiver->maxDepth = textMessage->getIntProperty(
					"maxDepth");
			crawlerInfoReceiver->taskId = textMessage->getIntProperty("taskId");
		} else {
			text = "NOT A TEXTMESSAGE!";
		}

		printf("Message #%d Received: %s\n", count, text.c_str());

	} catch (CMSException& e) {
		e.printStackTrace();
	}

	// 会话提交所有消息
	if (this->sessionTransacted) {
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
		delete crawlerInfoReceiver;
		crawlerInfoReceiver = NULL;

	} catch (CMSException& e) {
		e.printStackTrace();
	}
}

void SendSingleInfo(CrawlerInfo* infoNeedSent) {
	activemq::library::ActiveMQCPP::initializeLibrary();
    {
		std::string brokerURI =
        "failover:(tcp://112.124.49.59:61616"
//        "?wireFormat=openwire"
//        "&transport.useInactivityMonitor=false"
//        "&connection.alwaysSyncSend=true"
//        "&connection.useAsyncSend=true"
//        "?transport.commandTracingEnabled=true"
//        "&transport.tcpTracingEnabled=true"
//        "&wireFormat.tightEncodingEnabled=true"
        ")";

		bool useTopics = false;
		//bool sessionTransacted = false;
		int numMessages = 1;

		CrawlerInfoProducer producer(brokerURI, numMessages, useTopics);
		producer.setCrawlerInfo(infoNeedSent);

		// 开启生产线程
		Thread producerThread(&producer);
		producerThread.start();
		producerThread.join();
		producer.close();

	}
	activemq::library::ActiveMQCPP::shutdownLibrary();
}


//接收服务器消息，定义实际接收逻辑
CrawlerInfo RecvSingleInfo() {
	CrawlerInfo tmp;
	activemq::library::ActiveMQCPP::initializeLibrary();
    {
		std::string brokerURI =
        "failover:(tcp://112.124.49.59:61616"
//        "?wireFormat=openwire"
//        "&transport.useInactivityMonitor=false"
//        "&connection.alwaysSyncSend=true"
//        "&connection.useAsyncSend=true"
//        "?transport.commandTracingEnabled=true"
//        "&transport.tcpTracingEnabled=true"
//        "&wireFormat.tightEncodingEnabled=true"
        ")";

		bool useTopics = false;
		bool sessionTransacted = false;
		int numMessages = 1;

		CrawlerInfoConsumer consumer(brokerURI, numMessages, useTopics, sessionTransacted);

		// 开启消费线程
		Thread consumerThread(&consumer);
		consumerThread.start();

		//等待直至消费者准备完毕
		consumer.waitUntilReady();
		consumerThread.join();

		tmp = *(consumer.crawlerInfoReceiver);
		consumer.close();
	}
	activemq::library::ActiveMQCPP::shutdownLibrary();
	return tmp;
}




//定义队列发送接口
void SendListInfo(list<CrawlerInfo*> listInfo) {
	if( !listInfo.empty()) {
		list<CrawlerInfo*>::iterator itr;
		CrawlerInfo* tmpInfo;
		for( itr = listInfo.begin(); itr != listInfo.end(); ++itr) {
			tmpInfo = *itr;
			SendSingleInfo(tmpInfo);
		}
	}
}



//定义接收消息队列接口
list<CrawlerInfo> RecvListInfo(int size) {
	list<CrawlerInfo> tmpList;
	CrawlerInfo tmp;
	for( int i = 0; i < size; ++i) {
		tmp = RecvSingleInfo();
		tmpList.push_back(tmp);
	}
	return tmpList;
}

} // namespace activemq
