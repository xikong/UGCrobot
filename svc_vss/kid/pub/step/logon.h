/*
 * logon.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_LOGON_H_
#define KID_PUB_STEP_LOGON_H_

#include "message.h"
#include "fix.h"

namespace step {

class Logon : public Message {
 public:
  Logon();
  ~Logon();
 public:
  static std::string MsgType() { return kMsgTypeLogon; }
  const std::string* encrypt_method() const { return get(TAG_ENCRYPT_METHOD); }
  const std::string* version() const { return get(TAG_VERSION); }
  // 加密方法始终为0
  void set_encrypt_method(int method = 0) { set(TAG_ENCRYPT_METHOD, method); }
  void set_version(const std::string& version) { set(TAG_VERSION, version); }

  virtual std::string Encode();
 private:
  static const char version_[];
};

} /* namespace stock_logic */

#endif /* KID_PUB_STEP_LOGON_H_ */
