/*
 * logout.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_LOGOUT_H_
#define KID_PUB_STEP_LOGOUT_H_

#include "header.h"
#include "step.h"

namespace step {

class Logout : public Header {
 public:
  static const char kMsgFormatErr[];
  static const char kOutOfSysOpenTime[];
  static const char kInterfaceVerNotSupport[];
  static const char kDataLinkErr[];
  static const char kUnknownErr[];

  typedef std::map<std::string, std::string> LogoutCodeMap;
  static void Init();

 public:
  Logout(FieldMap& field_map);
  ~Logout();
 public:
  static std::string MsgType() { return kMsgTypeLogout; }
  const std::string* logout_code() const { return field_map_.get(TAG_LOGOUT_CODE); }
  std::string logout_msg() { return code_map_[*logout_code()]; }
  const std::string* text() const { return field_map_.get(TAG_TEXT); }
  std::string Encode();
 private:
  static LogoutCodeMap code_map_;
};

} /* namespace step */

#endif /* KID_PUB_STEP_LOGOUT_H_ */
