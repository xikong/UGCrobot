#include "activemq_client.h"
#include <list>

using namespace activemq_logic;

int main(int argc AMQCPP_UNUSED, char* argv[] AMQCPP_UNUSED) {

  //CrawlerInfo初始化
  activemq_logic::CrawlerInfo* crawler_info1 = new activemq_logic::CrawlerInfo();
  crawler_info1->cur_depth = 2;
  crawler_info1->max_depth = 6;
  crawler_info1->task_id = 157;
  crawler_info1->attr_id = 20;
  crawler_info1->URL = "http://www.google.com";

  //CrawlerInfo初始化
  activemq_logic::CrawlerInfo* crawler_info2 = new activemq_logic::CrawlerInfo();
  crawler_info2->cur_depth = 3;
  crawler_info2->max_depth = 7;
  crawler_info2->task_id = 159;
  crawler_info2->attr_id = 30;
  crawler_info2->URL = "http://www.baidu.com";

  //测试发送2个消息
  std::list<activemq_logic::CrawlerInfo*> list_sent;

  list_sent.push_back(crawler_info1);
  list_sent.push_back(crawler_info2);

  SendListInfo(list_sent);

  //定义接收3个消息，服务器消息队列只有2个，取出所有，延时等待后退出
  std::list<activemq_logic::CrawlerInfo> recv_list_info;
  recv_list_info = RecvListInfo(3);

  activemq_logic::CrawlerInfo tmpRecv;
  std::list<activemq_logic::CrawlerInfo>::iterator itr;
  for( itr = recv_list_info.begin(); itr != recv_list_info.end(); ++itr) {
    tmpRecv = *itr;
    std::cout << tmpRecv.URL << std::endl;
  }

  delete crawler_info1;
  crawler_info1 = NULL;

  delete crawler_info2;
  crawler_info2 = NULL;

  //system("pause");
}
