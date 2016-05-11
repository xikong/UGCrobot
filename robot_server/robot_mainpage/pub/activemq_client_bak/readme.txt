功能：提供两个接口： 
1. void SendListInfo(list<CrawlerInfo> ): 从客户端发送list<>信息至ativeMQ服务器

2. list<CrawlerInfo> RecvListInfo(int size): 从服务器取指定数量的消息返回客户端，如果取的数量大于服务器上队列剩余数量，那么则取完存在的消息，等待waitMillis(默认设置2000毫秒)后，如果没有新消息可以取，退出线程。

tips:
3. Makefile是基于test.cpp的，其中包含文件路径全部换成了绝对路径(192.168.0.2下的绝对路径)；

4. 实际activeMQ服务器的地址为 192.168.0.2:61616

5. activeMQ客户端安装编译环境在192.168.0.2 ：/home/runner/wangqian/ 下。
