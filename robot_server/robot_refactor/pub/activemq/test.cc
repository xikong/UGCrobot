#include "activemq_client.h"
#include <list>

using namespace activemq;

int main() {
	

	//CrawlerInfo初始化
	activemq::CrawlerInfo* crawler_info_1 = new activemq::CrawlerInfo();
	crawler_info_1->curDepth = 2;
	crawler_info_1->maxDepth = 6;
	crawler_info_1->taskId = 157;
	crawler_info_1->URL = "http://www.google.com";
	
	//CrawlerInfo初始化
	activemq::CrawlerInfo* crawler_info_2 = new activemq::CrawlerInfo();
	crawler_info_2->curDepth = 3;
	crawler_info_2->maxDepth = 7;
	crawler_info_2->taskId = 159;
	crawler_info_2->URL = "http://www.baidu.com";
	
	//测试发送2个消息
	std::list<activemq::CrawlerInfo*> list_be_sent;
	
	
	list_be_sent.push_back(crawler_info_1);
	list_be_sent.push_back(crawler_info_2);
   
	SendListInfo(list_be_sent);
	//std::cout << " num2 " << std::endl;

	//定义接收3个消息，服务器消息队列只有2个，取出所有，延时等待后退出
	std::list<activemq::CrawlerInfo> recv_list_info;
	recv_list_info = RecvListInfo(1);

	
	activemq::CrawlerInfo tmp_recv_info;
	std::list<activemq::CrawlerInfo>::iterator itr;
	for( itr = recv_list_info.begin(); itr != recv_list_info.end(); ++itr) {
		tmp_recv_info = *itr;
		std::cout << tmp_recv_info.URL << std::endl;
	}
	
	
	
	delete crawler_info_1;
	crawler_info_1 = NULL;

	delete crawler_info_2;
	crawler_info_2 = NULL;

	std::cout << "The test finished!" << std::endl;
	return 0;
}
