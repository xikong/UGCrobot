
#ifndef ROBOT_PUB_NET_COMM_HEAD_H_
#define ROBOT_PUB_NET_COMM_HEAD_H_

#include <list>
#include <string>

#include "../logic/logic_pub_comm.h"
#include "basic/basictypes.h"
#include "net/packet_processing.h"

using std::string;

#define _MAKE_HEAD(head, _packet_length, _is_zip_encrypt, _type, _checksum, \
        _operate_code, _bodylen,  _msg_id, _reserved)\
    do {\
        head.packet_length_ = _packet_length;\
        head.is_zip_encrypt_ = _is_zip_encrypt;\
        head.type_ = _type;\
        head.signature_ = _checksum;\
        head.operate_code_ = _operate_code;\
        head.data_length_ = _bodylen;\
        head.timestamp_ = time(NULL);\
        head.session_id_ = _msg_id;\
        head.reserved_ = _reserved;\
    }while(0)\

#define MAKE_HEAD(head, _operate_code,  _type, _is_zip_encrypt, \
        _check_sum, _reserved)\
    _MAKE_HEAD(head, 0, _is_zip_encrypt, _type, _check_sum, _operate_code, 0, 0, \
        _reserved)

#define DUMPHEAD()\
    PRINT_INT(packet_length);\
    PRINT_INT(is_zip_encrypt);\
    PRINT_INT(type);\
    PRINT_INT(signature);\
    PRINT_INT(operate_code);\
    PRINT_INT(data_length);\
    PRINT_INT(timestamp);\
    PRINT_INT64(session_id);\
    PRINT_INT(crawler_type);\
    PRINT_INT(crawler_id);\
    PRINT_INT(server_type);\
    PRINT_INT(server_id);\
    PRINT_INT(reserved);\

enum ePRS {
    NOZIP_AND_NOENCRYPT = 0,
    ZIP_AND_NOENCRYPT = 1,
    NOZIP_AND_ENCRYPT = 2,
    ZIP_AND_ENCRYPT = 3
};

enum eUAoRIP{
    IP = 1,
    UA = 2
};

enum eRegType{
    FAILED = 0,
    SUCCESS = 1
};

enum eServerType{
    LOGIN_SERVER = 1025,
    FORGERY_SERVER = 1026,
    MANAGER_SERVER = 1028
};

enum eRobotType{
    TASK_TIEBA = 7001,                  //贴吧
    TASK_WEIBO = 7002,                  //微博
    TASK_TIANYA = 7003,                 //BBS 天涯
    TASK_QQ = 7004,                     //QQ空间
    TASK_MOP = 7005,                    //maopu
    TASK_DOUBAN = 7006
};

enum eForgeryType{
    FORGERY_IP = 1,
    FORGERY_UA = 2,
    FORGERY_IP_UA = 3,
    NOT_FORGERY = 4
};

enum TASKSTAE {
    TASK_WAIT = 0,
    TASK_SEND = 1,
    TASK_RECV = 2,
    TASK_READY = 3,
    TASK_EXECUING = 4,
    TASK_STORAGE = 5,
    TASK_STORAGED = 6,
    TASK_EXECUED = 7,
    TASK_FAIL = 8,
    TASK_SUCCESS = 9
};

enum eMsgDefine{

    C2S_ROBOT_REQUEST_REGISTER = 1001,              //请求注册
    S2C_ROBOT_REGISTER_SUCCESS = 1002,              //注册结果

    C2S_ROBOT_REQUEST_UA_LIST = 1019,               //向服务器请求UA

    C2S_HEART_BEAT_CHECK_MSG = 1217,                //心跳包
    C2S_REPORT_STATE_MSG = 1222,                    //状态包

    C2R_REQUEST_LOGIN_ROUTER = 1224,                //请求登录router
    R2S_LOGIN_ROUTER_RESULT = 1226,                 //爬虫登录router结果

    S2C_ALLOCATING_WEIBO_MULTI_TASK = 1234,         //收到服务器分配微博任务
    C2S_FEEDBACK_TASK_STATUS = 1235,                //向服务器反馈任务状态

    S2C_ALLOCATING_TIANYA_MULTI_TASK = 1236,        //收到服务器分配的天涯任务
    S2C_ALLOCATING_TIEBA_MULTI_TASK = 1237,         //收到服务器分配的贴吧任务
    S2C_ALLOCATING_QQZONE_MULTI_TASK = 1238,        //收到服务器分配的qq空间任务


    S2C_ALLOCATING_MULTI_ROBOT_TASK = 1301

};

typedef struct SLBInfo{

    SLBInfo()
    : port_(-1)
    , conn_socket_(-1)
    , is_register_(FAILED){
    }

    std::string host_;
    int16       port_;
    int         conn_socket_;
    bool        is_register_;
    string      token_;

    int32       client_id_;
    int32       client_type_;
    string      mac_;
    string      passwd_;

} SLBAgent;

typedef struct RouterInfo{

    RouterInfo()
    : port_(0)
    , conn_socket_(-1){
    }

    string      host_;
    int16       port_;
    int         conn_socket_;
    int8        is_login_;
} RouterAgent;

#endif
