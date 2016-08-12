/*
 * realtime_session.cc
 *
 *  Created on: 2016年8月11日
 *      Author: zjc
 */

#include "realtime_session.h"
#include "glog/logging.h"
#include "logic/logic_unit.h"
#include "logon.h"
#include "logout.h"
#include "heartbeat.h"
#include "sys_param.h"
#include "stock_info.h"


namespace step {

RealtimeSession::RealtimeSession()
    : Session("realtime"),
      heart_int_(0),
      max_fail_int_(0),
      report_int_(0),
      stock_num_(0),
      index_num_(0),
      set_num_(0),
      sys_open_time_(0),
      sys_close_time_(0) {
  msg_handler_.insert(MsgHandler::value_type(Logon::MsgType(), boost::bind(&RealtimeSession::OnLogon, this)));
  msg_handler_.insert(MsgHandler::value_type(Logout::MsgType(), boost::bind(&RealtimeSession::OnLogout, this)));
  msg_handler_.insert(MsgHandler::value_type(Heartbeat::MsgType(), boost::bind(&RealtimeSession::OnHeartbeat, this)));
  msg_handler_.insert(MsgHandler::value_type(SysParam::MsgType(), boost::bind(&RealtimeSession::OnSysParam, this)));
  msg_handler_.insert(MsgHandler::value_type(StockInfo::MsgType(), boost::bind(&RealtimeSession::OnStockInfo, this)));
}

RealtimeSession::~RealtimeSession() {
}

bool RealtimeSession::OnConnect(int sock) {
  if (!Session::OnConnect(sock)) {
    LOG(ERROR) << "OnConnect error";
    return false;
  }
  FieldMap field_map;
  Logon logon(field_map);
  std::string buf = logon.Encode();
  DLOG(INFO) << "logon msg: " << escape(buf);
  if (!send_message(sock, buf.data(), buf.size())) {
    LOG(ERROR) << "send logon msg error";
    return false;
  }
  return true;
}

bool RealtimeSession::OnReceive(const char* buf, size_t len) {
  step_decoder_.Push(buf, len);
  while (Decoder::SUCCESS == step_decoder_.Parse(&msg_)) {
    const std::string* msg_type = msg_.get(TAG_MSG_TYPE);
    DLOG(INFO) << "revc " << *msg_type << " msg";
    MsgHandler::iterator it = msg_handler_.find(*msg_type);
    if (msg_handler_.end() == it) {
      LOG(ERROR) << "no handler to handle " << *msg_type << " msg";
      continue;
    }
    it->second();
  }
  return Session::OnReceive(buf, len);
}

void RealtimeSession::OnLogon() {
  Logon logon(msg_);
  heart_int_ = logon.heart_int();
  max_fail_int_ = logon.max_fail_int();
  report_int_ = logon.report_int();
}

void RealtimeSession::OnLogout() {
  Logout logout(msg_);
  LOG(FATAL) << "VDE logout, code: " << *logout.logout_code()
      << ", code msg: " << *logout.logout_msg()
      << ", reason: " << *logout.text();
}

void RealtimeSession::OnHeartbeat() {

}

void RealtimeSession::OnSysParam() {
  SysParam sys_param(msg_);
  stock_num_ = sys_param.stock_num();
  index_num_ = sys_param.index_num();
  set_num_ = sys_param.set_num();
  sys_open_time_ = sys_param.sys_open_time();
  sys_close_time_ = sys_param.sys_close_time();
}

void RealtimeSession::OnStockInfo() {
  const std::string* data = msg_.get(TAG_RAW_DATA);
  const char* first = data->c_str();
  const char* last = first + data->size();
  while (first < last) {
    mfast::message_cref msg = fast_decoder_.decode(first, last, true);
  }
}

} /* namespace step */
