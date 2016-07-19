/*
 * packet_define.h
 *
 *  Created on: 2016年4月7日
 *      Author: Harvey
 */

#ifndef UGCROBOT_MASTER_PUB_NET_PACKET_DEFINE_H_
#define UGCROBOT_MASTER_PUB_NET_PACKET_DEFINE_H_

#include <list>
#include <string>
#include "basic/basictypes.h"
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "logic/logic_comm.h"

#define PASSWORD_SIZE   8 + 1
#define MAC_SIZE        16 + 1
#define TOKEN_SIZE      32 + 1
#define SIGNATURE_SIZE  16 ＋ 1
#define STORAGE_INFO_SIZE 32 + 1
#define IP_FORGEN_SIZE     32 + 1
#define IP_SIZE         32 + 1
#define UA_FORGEN_SIZE   251 + 1
#define HADRINFO_SIZE   16 + 1
#define URL_SIZE        256 + 1
#define NAME_SIZE       32 + 1
#define KEY_SIZE        32 + 1

using std::string;

struct PacketHeadHex{
    int16 packet_length;    //整个包的长度
    int8 is_zip_encrypt;   //是否压缩
    int8 type;             //类型
    int16 checksum;         //检验包的值
    int16 operate_code;     //操作码
    int16 bodylen;          //包体长度
    int32 timestamp;        //当前时间
    int64 msg_id;           //消息id
    int16 crawler_type;     //爬虫类型
    int32 crawler_id;       //爬虫id
    int16 server_type;      //服务器类型
    int32 server_id;        //服务器id
    int32 reserved;         //保留字段
};

//包头长度
#define PACKET_HEAD_LENGTH (sizeof(int16) * 6 + sizeof(int8) * 2 + \
    sizeof(int32) * 4 + sizeof(int64))
struct PacketHead{

    PacketHead();
    virtual ~PacketHead();
    bool PackHead(const int32 packet_length );
    bool UnpackHead(const void* packet_stream, int32 len );

    virtual bool PackStream(void **packet_stream, int32 &packet_stream_length );

    int16 packet_length_;
    int8 is_zip_encrypt_;
    int8 type_;
    int16 signature_;
    int16 operate_code_;
    int16 data_length_;
    int32 timestamp_;
    int64 session_id_;
    int16 crawler_type_;
    int32 crawler_id_;
    int16 server_type_;
    int32 server_id_;
    int32 reserved_;

    packet::DataOutPacket *out_;
    packet::DataInPacket *in_;
};

//robot请求注册
#define ROBOT_REGISTER_INFO_SIZE (PACKET_HEAD_LENGTH + sizeof(int16) + PASSWORD_SIZE - 1 + MAC_SIZE - 1)
struct RobotRegisterInfo : public PacketHead{

    virtual bool PackStream(void **packet_stream, int32 &packet_stream_length );

    int16 level;
    char passwd[PASSWORD_SIZE];
    char mac[MAC_SIZE];
};

//Robot 注册结果
struct RobotRegisterSuccess : public PacketHead{

    bool UnpackStream(const void *packet_stream, int32 len );

    char router_ip[IP_SIZE];     //router地址
    int16 router_port;            //router端口
    char token[TOKEN_SIZE];      //分配的唯一token
};

// Robot 请求登录 Router
#define REG_LOGIN_ROUTER_SIZE (PACKET_HEAD_LENGTH + TOKEN_SIZE - 1)
struct RobotRequestLoginRouter : public PacketHead{

    virtual bool PackStream(void **packet_stream, int32 &packet_stream_length );

    char token[TOKEN_SIZE];
};

// 登录router结果
struct RobotLoginRouterResult : public PacketHead{

    bool UnpackStream(const void *packet_stream, int32 len );

    int8 is_success;
};

// Robot状态包
#define ROBOT_STATE_SIZE (PACKET_HEAD_LENGTH + sizeof(int32) * 2)
struct RobotStatePacket : public PacketHead{

    virtual bool PackStream(void **packet_stream, int32 &packet_stream_length );

    int32 max_task_num;
    int32 curr_task_num;
};

//所有任务的基类
struct TaskHead{

    TaskHead()
            : is_start_(0),
              is_finsh_(0),
              feed_server_id_(0),
              retry_times_(0) {
    }

    virtual ~TaskHead() {
    }

    bool UnpackTaskHead(packet::DataInPacket *in, int &temp );

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp ) = 0;

    int32 feed_server_id_;
    int64 task_id_;
    uint16 task_type_;
    int8 is_start_;
    int8 is_finsh_;
    int8 retry_times_;
    int64 cookie_id_;
    string cookie_;
    string content_;
    string forge_ip_;
    string forge_ua_;
    string pre_url_;
};

//微博任务
struct TaskWeiBoPacket : public TaskHead{
    string topic_id_;
    string host_uin_;

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//bbs天涯
struct TaskTianYaPacket : public TaskHead{
    uint64 pre_post_time_;
    string pre_title_;
    string pre_user_id_;
    string pre_user_name_;

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//贴吧任务
struct TaskTieBaPacket : public TaskHead{
    string kw_;
    string fid_;
    string tbs_;
    int32 floor_num_;
    string repost_id_;

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//QQ任务
struct TaskQQPacket : public TaskHead{
    uint64 host_uin_;
    string topic_id_;

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//猫扑任务
struct TaskMopPacket : public TaskHead{
    string pCatId_;
    string catalogId_;
    string fmtoken_;
    string currformid_;

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//豆瓣任务
struct TaskDouBanPacket : public TaskHead{

    virtual bool UnpackTaskBody(packet::DataInPacket *in, int &temp );
};

//收到服务器分配的任务
struct MultiTaskList : public PacketHead{

    std::list<struct TaskHead *> multi_task_list_;

    bool UnpackStream(const void *packet_stream, int32 len );
};

#define FEEDBACK_TASK_STATUS_SIZE   (PACKET_HEAD_LENGTH + sizeof(int8) + sizeof(int64) * 2)
struct FeedBackTaskStatus : public PacketHead{
    int16 task_type;
    int8 is_success;
    int64 task_id;
    int64 cookie_id;

    virtual bool PackStream(void **packet_stream, int32 &packet_stream_length );
};

static bool ReadDataByLen(string &data, int &temp, packet::DataInPacket *in );

#endif /* UGCROBOT_MASTER_PUB_NET_PACKET_DEFINE_H_ */
