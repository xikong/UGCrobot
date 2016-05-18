# 机器人回帖各平台说明
---

目前支持回帖平台如下：

> * [百度贴吧](http://tieba.baidu.com/)
> * [淘股吧](http://www.taoguba.com.cn/)
> * [雪球](https://xueqiu.com/2226355683)
> * [股吧](http://guba.eastmoney.com/)

关于通信协议见wiki说明： [机器人回帖wiki通信协议](http://wiki.smartdata-x.com/index.php/PluginsSvcRobotTask)

## kafka **topic** 说明

### 1.  


## 各平台回帖参数格式

### 公共参数

| 字段      |  类型   |  备注  |
| --------  | :-----: | :---- |
| plat_id   | int64  |  平台id |
| time   | string  |  当前可读时间字符串 |

### 1. 百度贴吧 （plat_id = 7001）

| 字段      |  类型   |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |
| kw        | string |  贴吧名称 |
| fid       | string |  帖子唯一id |
| floor_num | int32  |  回复楼层（若回复楼中楼需要） |
| repost_id | string |  回复楼层对应的唯一id |

### 2. 淘股吧 ( plat_id = 7007 )

| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| topicID   | string | 回帖话题id |
| subject   | string | 淘股吧帖子标题 |

### 3. 雪球 ( plat_id = 7008 )

| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  今日话题 url，或个人主页 url |
| topic_id  | string | 雪球个人主页,对应说说的 topic_id（如为空，则为回帖今日话题） |

### 4. 股吧 ( plat_id = 7009 )

| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |


------
作者 [@liuhongwei][1]     
2016 年 05月 09日    


[1]: http://weibo.com/5777025307/info
 
    
 
