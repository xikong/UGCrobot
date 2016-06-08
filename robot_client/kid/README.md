# 机器人回帖各平台说明
---

## 目前支持回帖平台如下

> * [百度贴吧](http://tieba.baidu.com/) 
> * [微博](http://www.weibo.com/)
> * [淘股吧](http://www.taoguba.com.cn/)
> * [雪球](https://xueqiu.com/2226355683)
> * [股吧](http://guba.eastmoney.com/)
> * [同花顺](http://www.10jqka.com.cn/)

关于回帖通信协议见wiki说明： [机器人回帖wiki通信协议](http://wiki.smartdata-x.com/index.php/PluginsSvcRobotTask)


## kafka **topic** 说明

> 1、  `robot_tiebacomment`  

> 最终解析出来的发帖需要的参数，由 spark 写， 机器人服务端读 

> 2、 `robot_stock`   

> 保存从主任务中解析出来的 url， 由 spark 写，机器人服务端读，机器人服务端读出来后，会把该任务分配给爬虫

> 3、 `robot_stockresult`

> 保存爬虫反爬取页面的结果，主要是 hbase 中的表名和 row key 信息，根据该信息可以找到爬虫爬取的页面内容。由机器人服务端写， spark 读


## 各平台回帖参数格式

###  公共参数
> | 字段      |  类型   |  备注  |
| --------  | :-----: | :---- |
| plat_id   | int64  |  平台id |
| time   | string  |  当前可读时间字符串 |


### 1. 百度贴吧 （plat_id = 7001）
>| 字段      |  类型   |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |
| kw        | string |  贴吧名称 |
| fid       | string |  帖子唯一id |
| floor_num | int32  |  回复楼层（若回复楼中楼需要） |
| repost_id | string |  回复楼层对应的唯一id |

### 2. 微博 ( plat_id = 7002 )

>| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| mid  | string | 每条微博对应的id |
| ouid  | string | 该条微博的用户id |

### 3. 淘股吧 ( plat_id = 7007 )

>| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| topicID   | string | 回帖话题id |
| subject   | string | 淘股吧帖子标题 |

### 4. 雪球 ( plat_id = 7008 )

>| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  今日话题 url，或个人主页 url |
| topic_id  | string | 雪球个人主页,对应说说的 topic_id（如为空，则为回帖今日话题） |

### 5. 股吧 ( plat_id = 7009 )

>| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |

### 6. 同花顺 ( plat_id = 7010 )

>| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |

------
作者 [@liuhongwei][1]     
2016 年 05月 23日    


[1]: http://weibo.com/5777025307/info
 
    
 
