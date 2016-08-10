#ifndef FIX_FIX_H_
#define FIX_FIX_H_

#include "message.h"
#include "decoder.h"

const int TAG_INVALID_FIELD     = -1;
// STEP HEADER
const int TAG_BEGIN_STRING      = 8;
const int TAG_BODY_LENGTH       = 9;
const int TAG_MSG_TYPE          = 35;
const int TAG_SENDER_COMP_ID    = 49;
const int TAG_TARGET_COMP_ID    = 56;
const int TAG_MSG_SEQ_NUM       = 34;
const int TAG_SENDING_TIME      = 52;
const int TAG_MESSAGE_ENCODING  = 347;

// Logon
const int TAG_ENCRYPT_METHOD    = 98;
const int TAG_VERSION           = 8936;
const int TAG_HEART_BT_INT      = 108;
const int TAG_MAX_FAIL_INT      = 8937;
const int TAG_REPORT_INT        = 8938;

// Logout
/**
 * E0001 = 消息格式错误
 * E0002 = 非系统开放时间
 * E0003 = 非系统支持接口版本
 * E0004 = 数据链接故障(包括心跳监测失败)
 * E9999 = 其它未列名错误
 */
const int TAG_LOGOUT_CODE       = 8939;
const int TAG_TEXT              = 58;

// Resend
/**
 * 重发种类
 * 1 = 证券信息
 * 2 = 逐笔成交数据
 * 3 = 逐笔委托数据
 * 4 = 公告消息
 * 5 = 增值信息
 */
const int TAG_RESEND_TYPE       = 10077;
const int TAG_RESEND_METHOD     = 10075;


const int TAG_RAW_DATA_LENGTH   = 95;   // FAST 消息部分数据长度
const int TAG_RAW_DATA          = 96;   // FAST 消息体

const int TAG_CHECK_SUM         = 10;

///////////////////////////////////////////////////////////////////////////////

const char kBeginString[]    = "STEP.1.0.0";
const char kStepHeaderFlag[] = "8=STEP.1.0.0";
///////////////////////////////////////////////////////////////////////////////

const char kMsgTypeLogon[]        = "A";      // VSS -> VDE, VDE -> VSS
const char kMsgTypeLogout[]       = "5";      // VSS -> VDE, VDE -> VSS
const char kMsgTypeHeartbeat[]    = "UA001";  // VSS -> VDE, VDE -> VSS
const char kMsgTypeSysParam[]     = "UA002";  // VDE -> VSS
const char kMsgTypeUserReport[]   = "UA003";  // VDE -> VSS
const char kMsgTypeStockInfo[]    = "UA101";
const char kMsgTypeStockStatus[]  = "UA102";
const char kMsgTypeSnapshot[]     = "UA103";
const char kMsgTypeIndex[]        = "UA104";
const char kMsgTypeOrder[]        = "UA201";
const char kMsgTypeTrade[]        = "UA202";
const char kMsgTypeBulletin[]     = "UA401";
const char kMsgTypeInformation[]  = "UA402";
const char kMsgTypeResend[]       = "2";

///////////////////////////////////////////////////////////////////////////////

// step head: "8=STEP.1.0.0<SOH>"
const int STEP_HEAD_SIZE = 13;
// step tail: "10=XXX<SOH>"
const int STEP_TAIL_SIZE = 7;

//
const int STEP_PACKET_MIN_SIZE = STEP_HEAD_SIZE + STEP_TAIL_SIZE;

///////////////////////////////////////////////////////////////////////////////
// Message Field Order
const int FIELD_ORDER_HEAD[] = {
    TAG_BEGIN_STRING,
    TAG_BODY_LENGTH,
    TAG_MSG_TYPE,
    TAG_SENDER_COMP_ID,
    TAG_TARGET_COMP_ID,
    TAG_MSG_SEQ_NUM,
    TAG_SENDING_TIME
};

const int FIELD_ORDER_TAIL[] = {
    TAG_CHECK_SUM
};

// Logon
const int FIELD_ORDER_LOGON[] = {
    TAG_ENCRYPT_METHOD,
    TAG_VERSION,
    TAG_HEART_BT_INT,
    TAG_MAX_FAIL_INT,
    TAG_REPORT_INT,
    TAG_INVALID_FIELD
};

// Logout
const int FIELD_ORDER_LOGOUT[] = {
    TAG_LOGOUT_CODE,
    TAG_TEXT,
    TAG_INVALID_FIELD
};

// Heartbeat
const int FIELD_ORDER_HEARTBEAT[] = {
    TAG_INVALID_FIELD
};

// Resend
const int FIELD_ORDER_RESEND[] = {
    TAG_RESEND_TYPE,
    TAG_RESEND_METHOD,
    TAG_INVALID_FIELD
};

///////////////////////////////////////////////////////////////////////////////
namespace step {

	const static char SEPARATE_BYTE = '=';
	const static char STOP_BYTE = '\x01';

}; // namespace fix

#endif
