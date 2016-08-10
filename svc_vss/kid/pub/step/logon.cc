/*
 * logon.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include "logon.h"
#include "fix.h"

namespace step {

const char Logon::version_[] = "1.00";

Logon::Logon() {
}

Logon::~Logon() {
}

std::string Logon::Encode() {
  set(TAG_MSG_TYPE, kMsgTypeLogon);
  if (NULL == get(TAG_ENCRYPT_METHOD)) {
    set(TAG_ENCRYPT_METHOD, 0);
  }
  if (NULL == get(TAG_VERSION)) {
    set(TAG_VERSION, version_);
  }
  return Message::Encode();
}

} /* namespace stock_logic */
