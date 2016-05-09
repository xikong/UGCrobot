# 机器人回帖各平台说明

[TOC]

---

目前支持回帖平台如下：

> * [百度贴吧](http://tieba.baidu.com/)
> * [淘股吧](http://www.taoguba.com.cn/)

关于通信协议见wiki说明：

## [机器人回帖wiki通信协议](http://wiki.smartdata-x.com/index.php/PluginsSvcRobotTask)

---

## 各平台回帖参数格式

### 1. 百度贴吧

| 字段      |  类型   |  备注  |
| --------  | :-----: | :---- |
| pre_url   | string |  回帖地址（原帖url） |
| kw        | string |  贴吧名称 |
| fid       | string |  帖子唯一id |
| floor_num | int32  |  回复楼层（若回复楼中楼需要） |
| repost_id | string |  回复楼层对应的唯一id |

### 2. 淘股吧

| 字段    |  类型  |  备注  |
| --------  | :-----: | :---- |
| topicID   | string | 回帖话题id |
| subject   | string | 淘股吧帖子标题 |


------
作者 [@liuhongwei][1]     
2016 年 05月 09日    


[1]: http://weibo.com/5777025307/info
 
    
 
