/*
 * logon.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_LOGON_H_
#define KID_PUB_STEP_LOGON_H_

#include "header.h"
#include "step.h"

namespace step {

class Logon : public Header {
 public:
  Logon(FieldMap& field_map);
  ~Logon();
 public:
  static std::string MsgType() { return kMsgTypeLogon; }
  const std::string* encrypt_method() const { return field_map_.get(TAG_ENCRYPT_METHOD); }
  const std::string* version() const { return field_map_.get(TAG_VERSION); }
  int heart_int() const { return atoi(field_map_.get(TAG_HEART_BT_INT)->c_str()); }
  int max_fail_int() const { return atoi(field_map_.get(TAG_MAX_FAIL_INT)->c_str()); }
  int report_int() const { return atoi(field_map_.get(TAG_REPORT_INT)->c_str()); }
  // 加密方法始终为0
  void set_encrypt_method(int method = 0) { field_map_.set(TAG_ENCRYPT_METHOD, method); }
  void set_version(const std::string& version) { field_map_.set(TAG_VERSION, version); }

  std::string Encode();
 private:
  static const char version_[];
};

} /* namespace stock_logic */

#endif /* KID_PUB_STEP_LOGON_H_ */
