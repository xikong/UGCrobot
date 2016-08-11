/*
 * logout.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include "logout.h"
#include "glog/logging.h"

namespace step {

Logout::LogoutCodeMap Logout::code_map_;

const char Logout::kMsgFormatErr[] = "E0001";
const char Logout::kOutOfSysOpenTime[] = "E0002";
const char Logout::kInterfaceVerNotSupport[] = "E0003";
const char Logout::kDataLinkErr[] = "E0004";
const char Logout::kUnknownErr[] = "E9999";

void Logout::Init() {
  code_map_.insert(LogoutCodeMap::value_type(kMsgFormatErr, "消息格式错误"));
  code_map_.insert(LogoutCodeMap::value_type(kOutOfSysOpenTime, "非系统开放时间"));
  code_map_.insert(LogoutCodeMap::value_type(kInterfaceVerNotSupport, "非系统支持接口版本"));
  code_map_.insert(LogoutCodeMap::value_type(kDataLinkErr, "数据链路故障(包括心跳监测失败)"));
  code_map_.insert(LogoutCodeMap::value_type(kUnknownErr, "其他未列明错误"));
}

Logout::Logout(const std::string& logout_code, const std::string& text) {
  if (0 == code_map_.count(logout_code)) {
    LOG(ERROR) << "unknown logout code";
  }
  set(TAG_MSG_TYPE, kMsgTypeLogout);
  set(TAG_LOGOUT_CODE, logout_code);
  set(TAG_TEXT, text);
}

Logout::~Logout() {
  // TODO Auto-generated destructor stub
}

std::string Logout::Encode() {
  return FieldMap::Encode();
}

} /* namespace step */
