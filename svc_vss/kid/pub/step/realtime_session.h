/*
 * realtime_session.h
 *
 *  Created on: 2016年8月11日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_REALTIME_SESSION_H_
#define KID_PUB_STEP_REALTIME_SESSION_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "session.h"
#include "decoder.h"

namespace step {

class RealtimeSession : public Session {
 public:
  typedef boost::function<void()> Callback;
  typedef std::map<std::string, Callback> MsgHandler;
  RealtimeSession();
  ~RealtimeSession();
 public:
  virtual bool OnConnect(int sock);
  virtual bool OnDisconnect();
  virtual bool OnReceive(const char* buf, size_t len);
 public:
  void OnLogon();
  void OnLogout();
  void OnHeartbeat();
  void OnSysParam();
  void OnStockInfo();
 private:
  MsgHandler msg_handler_;
  Decoder step_decoder_;
  FieldMap msg_;
  int heart_int_;  // 心跳间隔
  int max_fail_int_;  // 最大连续心跳监测失败次数
  int report_int_;    // 上报用户数目的时间间隔,以分为单位
  int stock_num_;     // 证券数目
  int index_num_;
  int set_num_;
  int sys_open_time_;
  int sys_close_time_;

};

} /* namespace step */

#endif /* KID_PUB_STEP_REALTIME_SESSION_H_ */
