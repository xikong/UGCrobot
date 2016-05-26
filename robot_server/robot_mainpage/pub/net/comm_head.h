//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月7日 Author: kerry

#ifndef KID_PUB_NET_COMM_HEAD_H_
#define KID_PUB_NET_COMM_HEAD_H_

#include <list>
#include <string>

#include "basic/basictypes.h"
#include "logic/logic_comm.h"

#define PASSWORD_SIZE   8 + 1
#define MAC_SIZE        16 + 1
#define TOKEN_SIZE      32 + 1
#define SIGNATURE_SIZE  16 ＋ 1
#define STORAGE_INFO_SIZE 32 + 1
#define IP_FORGEN_SIZE     31 + 1
#define UA_FORGEN_SIZE   251 + 1
#define HADRINFO_SIZE   16 + 1
#define URL_SIZE        256 + 1
#define NAME_SIZE       32 + 1
#define KEY_SIZE        32 + 1

enum ClientType {
	CRAWLER = 1,
	SLB = 2,
	ROUTER = 3,

	PLUGIN_SERVER = 0x400,
	LOGIN_SERVER = 0x401,
	FORGERY_SERVER = 0x402,
	STORAGER_SERVER = 0x403,
	TASK_SCHEDULE_SERVER = 0x404,
	TASK_DETAIL_SERVER = 0x405,

	VERIFYSERVER = 5
};

enum PRS {
	NOZIP_AND_NOENCRYPT = 0,
	ZIP_AND_NOENCRYPT = 1,
	NOZIP_AND_ENCRYPT = 2,
	ZIP_AND_ENCRYPT = 3
};

enum operatorcode {
	HEART_PACKET = 0x64,

	HEART_TO_SLB = 0x4C3,	// 1219, 心跳包
	CRAWLER_MGR_REG = 0x3E9,
	CRAWLER_MGR_REGSTATE = 0x3EA,
	GET_HARDINFO = 0x3EB,
	REPLY_HARDINFO = 0x3EC,
	CRAWLER_REG_FAILED = 0x3ED,

	REGISTER_SERVER = 0x4BB,		// 1211, PlubinSVC 注册
	PLUGIN_SVC_REG_STATE = 0x4BC,	// 1212, PluginSVC 管理注册成功
	PLUGIN_SVC_MGR_ROUTER_REG = 0x4CB,	// 1227, Router登录PluginSVC
	REPLY_ROUTER_REG = 0x4CC,
	PLUGIN_SVC_MGR_ROUTER_REG_STATE = 0x4CE,	// 1230, SLB返回Router验证信息

	PLUGIN_SVC_AVAILABLE_RESOURCE_NUM = 0x4CF,	// 1231, PluginSVC状态包

	ASSIGNMENT_SINGLE_TASK = 0x3F5,
	ASSIGNMENT_MULTI_TASK = 0x3F6,	// 1014, 向客户端分配任务
	ASSIGNMENT_DETAIL_TASK = 0x42E,

	REPLY_TASK_STATE = 0x3F7,// 1015, Crawler--->RouterSVC--->PluginSVC 向服务端反馈任务状态
	REPLY_DETAIL_STATE = 0x42F,

	ROUTER_STATUS = 0x4C5,		// 1221, Router统计爬虫任务状态包1.0

	GET_CRAWL_CONTENT_NUM = 0x3F8,
	REPLY_CRAWL_CONTENT_NUM = 0x3F9,

	CRAWL_HBASE_STORAGE_INFO = 0x3FA,// 1018, Crawler--->RouterSVC--->PluginSVC, 向服务端提交爬取内容

	GET_FORGEINFO = 0x3FB,	 // 1019, 获取UA、IP, Crawler--->RouterSVC--->PluginSVC
	REPLY_FOGEINFO_IP = 0x3FC,	    // 1020, 向客户端发送IP
	REPLY_FOGEINFO_UA = 0X3FD,	    // 1021, 向客户端发送UA
	CRAWL_FTP_STORAGE_INFO = 0x3FE,

	ANALYTICAL_INFO = 0x406,
	ANALYTICAL_REG = 0x407,
	ANALYTICAL_STATE = 0x408,
	ANALYTICAL_URL_SET = 0x409,

	LOGIN_REQUIRE_COOKIES = 0x41A,// 1050, 向服务端获取cookie, Crawler--->RouterSVC--->PluginSVC
	SPECIAL_PREASSIGN_COOKIES = 0x41B,
	DELIVERED_COOKIE_SET = 0x41C,	// 1052, 向客户端发送cookie
	COMMON_PREASSIGN_COOKIES = 0x41D,

	CRAWLER_AVAILABLE_RESOURCE_NUM = 0x424, // 1060, 向服务器反馈可承载任务个数

	TEMP_CRAWLER_OP = 20000,
	TEMP_ANALYTICAL = 30000,

	// 机器人任务
	ASSIGN_WEIBO_TASK = 0x4D2,		// 1234, 向机器人分配微博任务
	ROBOT_TASK_STATE = 0x4D3,		// 1235, 机器人任务执行结果

	ASSIGN_TIANYA_TASK = 0x4D4,		// 1236, 天涯
	ASSIGN_TIEBA_TASK = 1438,		// 1438, 个人主页任务
	ASSIGN_QZONE_TASK = 0x4D6		// 1238, qq 空间留言
};

//  packet_length 长度为原始数据长度
struct PacketHead {
	int16 packet_length;
	int8 is_zip_encrypt;
	int8 type;
	int16 signature;
	int16 operate_code;
	int16 data_length;
	int32 timestamp;
	int64 session_id;
	int16 crawler_type;
	int32 crawler_id;
	int16 server_type;
	int32 server_id;
	int32 reserved;
};

enum VerifyState {
	VERIFY_STATE_UNVERIFIED, VERIFY_STATE_SUCCESS, VERIFY_STATE_FAILED
};

#define PACKET_HEAD_LENGTH (sizeof(int16) * 6 + sizeof(int8) * 2 + \
    sizeof(int32) * 4 + sizeof(int64))

//  CRAWLER_MGR_REG = 1001
#define CRAWLER_MGR_REG_SIZE (sizeof(int16) + PASSWORD_SIZE - 1 + MAC_SIZE - 1)
struct CrawlerMgrReg: public PacketHead {
	int16 level;
	char password[PASSWORD_SIZE];
	char mac[MAC_SIZE];
};

// ROUTER_STATUS = 0x4C5, 1221
#define ROUTER_STATUS_UNIT_SIZE	(sizeof(int16) + sizeof(uint32) * 2)
struct RouterStatusUnit {
	int16 crawler_type;
	uint32 max_tasks;
	uint32 cur_tasks;
};

struct RouterStatus: public PacketHead {
	int32 router_id;
	std::list<struct RouterStatusUnit *> status_list;
};

#define PLUGIN_SVC_REG_SIZE (sizeof(int16) * 2 + PASSWORD_SIZE-1 + MAC_SIZE-1)
struct RegisterServer: public PacketHead {
	int16 level;
	char password[PASSWORD_SIZE];
	char mac[MAC_SIZE];
	uint16 port;
};

#define PLUGIN_SVC_REG_STATE_SIZE  (TOKEN_SIZE-1)
struct PluginSvcRegState: public PacketHead {
	char token[TOKEN_SIZE];
};

#define PLUGIN_SVC_ROUTER_REG_SIZE  (sizeof(uint32) + TOKEN_SIZE-1)
struct PluginSvcMgrRouterReg: public PacketHead {
	char token[TOKEN_SIZE];
	uint32 router_id;
};

struct PluginSvcMgrRouterRegState: public PacketHead {
	uint32 router_id;
	uint8 state;
};

#define REPLY_ROUTER_REG_SIZE	(sizeof(uint8))
struct ReplyRouterReg: public PacketHead {
	uint8 state;
};

// PLUGIN_SVC_MGR_TASK_STATE = 1015
struct PluginSvcMgrTaskState: public PacketHead {
	uint64 task_id;
	uint8 task_state;		// 4: 任务执行中, 7: 任务完成
};

// PLUGIN_SVC_AVAILABLE_RESOURCE_NUM = 1231
#define REPLY_PLG_SVC_AVAILABLE_TASKS	(sizeof(int32) * 2)
struct ReplyPlgSvcTaskState: public PacketHead {
	uint32 max_tasks;
	uint32 cur_tasks;
};

//  ANALYTICAL_REG = 1031
struct AnalyticalMgrReg: public CrawlerMgrReg {
};

//  CRAWLER_MGR_REGSTATE = 1002
#define CRAWLER_MGR_REG_STATE_SIZE (sizeof(int32) + TOKEN_SIZE - 1)
struct CrawlerMgrRegState: public PacketHead {
	int32 id;
	char token[TOKEN_SIZE];
};

//  GET_HARDINFO = 1003 PACKHEAD

//  REPLY_HARDINFO = 1004
#define REPLY_HARDINFO_SIZE (sizeof(int32) +\
        TOKEN_SIZE - 1 + (HADRINFO_SIZE - 1) * 2)\

struct ReplyHardInfo: public PacketHead {
	int32 id;
	char token[TOKEN_SIZE];
	char cpu[HADRINFO_SIZE];
	char mem[HADRINFO_SIZE];
};

//  CRAWLER_FAILED = 1005
#define CRAWLER_FAILED_SIZE (sizeof(int32))
struct CrawlerFailed: public PacketHead {
	int32 erron_code;
};

//  ASSIGNMENT_SINGLE_TASK = 1013
#define ASSIGNMENT_SINGLE_TASK_SIZE (sizeof(int32)\
        + sizeof(64) + sizeof(8) * 2 + URL_SIZE -1)
struct AssignmentSingleTask: public PacketHead {
	int32 id;
	int64 task_id;
	int8 depth;
	int8 machine;
	int8 storage;
	char url[URL_SIZE];
};

#define TASK_UNIT_SIZE (sizeof(int64) * 3 + sizeof(int8) * 8\
         + sizeof(int16))
struct TaskUnit {
	int16 len;
	int64 task_id;
	int64 attr_id;
	int64 unix_time;
	int8 max_depth;
	int8 current_depth;
	int8 machine;
	int8 storage;
	int8 is_login;
	int8 is_over;
	int8 is_forge;
	int8 method;
	char url[URL_SIZE];
};

//  ASSIGNMENT_MULTI_TASK = 1014
struct AssignmentMultiTask: public PacketHead {
	typedef std::list<struct TaskUnit*> TaskUnitList;
	std::list<struct TaskUnit*> task_set;

	~AssignmentMultiTask() {
		LOG_DEBUG("call AssignmentMultiTask Destructor");
		TaskUnitList::iterator it = task_set.begin();
		for (; it != task_set.end(); ++it) {
			LOG_DEBUG2("delete task unit: %X", *it);
			delete *it;
		}
	}
};

//  ASSIGNMENT_DETAIL_TASK = 1070
struct AssignmentDetailTask: public PacketHead {
	int32 id;
	std::list<struct TaskUnit*> task_set;
};

// ****************************** Robot Task Begin ******************************
//  ASSIGN_WEIBO_TASK = 1234
#define WEIBO_TASK_UNIT_SIZE	(sizeof(uint64) * 2 + sizeof(uint16) * 5)
struct WeiboTaskUnit {
	uint64		task_id;
	uint64		cookie_id;
	uint16		cookie_len;
	std::string	cookie;	
	uint16		content_len;
	std::string	content;
	uint16		addr_len;
	std::string	addr;
	uint16		topic_id_len;
	std::string	topic_id;
	uint16		host_uin_len;
	std::string	host_uin;
};

#define WEIBO_TASK_SIZE	(sizeof(uint16))
struct AssingWeiboTask: public PacketHead {
	uint16	tasks_num;
	std::list<struct WeiboTaskUnit*>	task_set;
	AssingWeiboTask(): tasks_num(0) {
	}
};

//  ROBOT_TASK_STATE = 1235
struct RobotTaskStatus: public PacketHead {
	int16	task_type;
	int8	is_success;
	int64	task_id;
	int64	cookie_id;
};

//  ASSIGN_QZONE_TASK = 1236
#define QZONE_TASK_UNIT_SIZE	(sizeof(uint64) * 3 + sizeof(uint16) * 4)
struct QzoneTaskUnit {
	uint64		task_id;
	uint64		cookie_id;
	uint16		cookie_len;
	std::string	cookie;
	uint16		content_len;
	std::string	content;
	uint16		addr_len;
	std::string	addr;
	uint64		host_uin;
	uint16		topic_id_len;
	std::string	topic_id;
};

#define QZONE_TASK_SIZE	(sizeof(uint16))
struct AssignQzoneTask: public PacketHead {
	uint16	tasks_num;
	std::list<struct QzoneTaskUnit*>	task_set;
	AssignQzoneTask(): tasks_num(0) {}
};

//  ASSIGN_TIANYA_TASK = 1236
#define TIANYA_TASK_UNIT_SIZE	(sizeof(uint64) * 3 + sizeof(uint16) * 7)
struct TianyaTaskUnit {
	uint64		task_id;
	uint64		cookie_id;
	uint16		cookie_len;
	std::string	cookie;
	uint16		content_len;
	std::string	content;
	uint16		addr_len;
	std::string	addr;
	uint64		pre_post_time;
	uint16		pre_url_len;
	std::string	pre_url;
	uint16		pre_title_len;
	std::string	pre_title;
	uint16		pre_user_id_len;
	std::string	pre_user_id;
	uint16		pre_user_name_len;
	std::string	pre_user_name;

};

#define TIANYA_TASK_SIZE	(sizeof(uint16))
struct AssignTianyaTask: public PacketHead {
	uint16	tasks_num;
	std::list<struct TianyaTaskUnit*>	task_set;
	AssignTianyaTask(): tasks_num(0) {}
};

//  ASSIGN_TIEBA_TASK = 1438
#define TIEBA_TASK_UNIT_SIZE	(sizeof(uint64) * 2 + sizeof(uint16) * 7)
struct TiebaTaskUnit {
	uint16 task_len;
	uint16 task_type;
	uint64 task_id;
	uint16 row_key_len;
	std::string row_key;
	uint64 cookie_id;
	uint16 cookie_len;
	std::string cookie;
	uint16 addr_len;
	std::string addr;
	uint16 ua_len;
	std::string ua;
	uint16 url_len;
	std::string url;
};

#define TIEBA_TASK_SIZE	(sizeof(uint16))
struct AssignTiebaTask: public PacketHead {
	uint16	tasks_num;
	std::list<struct TiebaTaskUnit*>	task_set;
	AssignTiebaTask(): tasks_num(0) {}
};
// ****************************** Robot Task end  ******************************
//  REPLY_TASK_STATE = 1015
#define REPLYTASKTATE_SIZE (sizeof(int64) + sizeof(int8))
struct ReplyTaskState: public PacketHead {
	int64 jobid;
	int8 state;
};

//  REPLY_DETAIL_STATE = 1071
#define REPLYDETAILTATE_SIZE (sizeof(int32) + TOKEN_SIZE - 1\
    + sizeof(int64) + sizeof(int8))
struct ReplyDetailState: public PacketHead {
	int32 id;
	char token[TOKEN_SIZE];
	int64 jobid;
	int8 state;
};

//  GET_CRAWL_CONTENT_NUM =1016
#define CRAWLCONTENTNUM_SIZE (sizeof(int32) + sizeof(int64))
struct CrawlContentNum: public PacketHead {
	int32 id;
	int64 jobid;
};

//  REPLY_CRAWL_CONTENT_NUM = 1017
#define REPLYCRAWLCONTENTNUM_SIZE (sizeof(int32) * 2 + sizeof(int64)\
    + TOKEN_SIZE - 1)
struct ReplyCrawlContentNum: public PacketHead {
	int32 id;
	char token[TOKEN_SIZE];
	int64 task_id;
	int32 num;
};

#define TABLE_NAME_SIZE	32 + 1
#define CRAWLSTORAGEINFO_SIZE (sizeof(int16) + sizeof(int64) * 2\
							   + sizeof(int8) * 2 + TABLE_NAME_SIZE-1)
#define ROWKEY_SIZE(unit_size)	((unit_size)-CRAWLSTORAGEINFO_SIZE)
struct StorageUnit {
	int16 len;
	int64 task_id;
	int64 attr_id;
	int8 max_depth;
	int8 cur_depth;
	char table_name[TABLE_NAME_SIZE];
	std::string rowkey;
};

//  CRAWL_HBASE_STORAGE_INFO = 1018 //CRAWL_FTP_STORAGE_INFO = 1019
struct CrawlStorageInfo: public PacketHead {
	std::list<struct StorageUnit*> storage_set;
};

//  GET_FORGEINFO = 1019
#define GETFORGEINFO_SIZE (sizeof(int64) + sizeof(int8) * 2)
struct GetForgeInfo: public PacketHead {
	int64 task_id;
	int8 forge_type;
	int8 num;
};

//  REPLY_FOGEINFO_IP = 1020
#define IPFORGEINFO_SIZE (sizeof(uint16) + sizeof(int32) + sizeof(int8))
struct IPForgeInfo {
	uint16 len;
	int32 id;
	int8 type;
	char forgen_info[IP_FORGEN_SIZE];
};
struct ReplyIPForgeInfo: public PacketHead {
	int64 task_id;
	std::list<struct IPForgeInfo*> forgen_set;
};

//  REPLY_FOGEINFO_UA = 1021
#define UAFORGEINFO_SIZE (sizeof(uint16) + sizeof(int32) + sizeof(int8))
struct UAForgeInfo {
	uint16 len;
	int32 id;
	int8 type;
	char forgen_info[UA_FORGEN_SIZE];
};
struct ReplyUAForgeInfo: public PacketHead {
	int64 task_id;
	std::list<struct UAForgeInfo*> forgen_set;
};

#define ANALYIUNIT_SIZE  ((NAME_SIZE - 1) + (KEY_SIZE - 1)\
    + sizeof(int64) * 2 + sizeof(int32) + sizeof(int8) * 3 )
struct AnalysiUnit {
	int64 ayalysi_id;
	int64 task_id;
	int32 attr_id;
	int8 type;
	int8 max_depth;
	int8 cur_depth;
	char name[NAME_SIZE];
	char key[KEY_SIZE];
};

//  ANALYTICAL_INFO
struct AnalysiInfo: public PacketHead {
	std::list<struct AnalysiUnit*> analysi_set;
};

//  ANALYTICAL_STATE
#define ANALYSISTATE_SIZE (sizeof(int64) + sizeof(int8))
struct AnalysiState: public PacketHead {
	int64 id;
	int8 state;
};

//  ANALYTICAL_URL_SET = 1033
#define ANALYTICAL_URL_UNIT_SIZE (sizeof(int8) * 3\
        +sizeof(int64) * 2 + URL_SIZE - 1)
struct AnalyticalURLUnit {
	int64 task_id;
	int64 attr_id;
	int8 max_depth;
	int8 current_depth;
	int8 method;
	char url[URL_SIZE];
};

struct AnalyticalURLSet: public PacketHead {
	int32 id;
	char token[TOKEN_SIZE];
	std::list<AnalyticalURLUnit*> analytical_url_set;
};

#define REQUIRE_LOGIN_COOKIE_LENGTH (32 + sizeof(int64) + \
    sizeof(int32) + sizeof(int8))

struct LoginCookieUnit {
	int16 len;
	std::string login_cookie_body;
	//std::string login_last_time;
};

struct LoginCookieSet: public PacketHead {
	int32 manage_id;
	int64 attr_id;
	std::list<LoginCookieUnit*> login_cookie_set;
};

struct RequireLoginCookie: public PacketHead {
	int32 manage_id;		// 平台类别
	int64 attr_id;			// 平台 id
	int8 amount;			// 获取 cookie 个数
};

#define CRAWLER_AVAILABLE_RESOURCE_NUM_SIZE (sizeof(int32) + sizeof(int16))
struct CrawlerAvailableResourceNum: public PacketHead {
	int16 task_num;
};

#endif /*KID_PUB_NET_COMM_HEAD_H_*/
