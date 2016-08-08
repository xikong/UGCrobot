#ifndef ROBOT_PUB_NET_COMM_HEAD_H_
#define ROBOT_PUB_NET_COMM_HEAD_H_

#include <list>
#include <string>

#include "logic/logic_comm.h"
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

enum eRegType {
    FAILED = 0,
    SUCCESS = 1
};

enum eRobotType {
    TASK_TIEBA = 7001,                  //贴吧
    TASK_WEIBO = 7002,                  //微博
    TASK_TIANYA = 7003,                 //BBS 天涯
    TASK_QQ = 7004,                     //QQ空间
    TASK_MOP = 7005,                    //maopu
    TASK_DOUBAN = 7006,					//豆瓣
    TASK_TAOGUBA = 7007,				//淘股吧
    TASK_XUEQIU = 7008,					//雪球
    TASK_IGUBA = 7009,                  //东方股吧
    TASK_TONGHUASHUN = 7010,            //同花顺
};

#endif
