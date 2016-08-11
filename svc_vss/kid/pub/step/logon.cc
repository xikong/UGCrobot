/*
 * logon.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include "logon.h"

namespace step {

const char Logon::version_[] = "1.00";

Logon::Logon(FieldMap& field_map)
    : Header(field_map) {
}

Logon::~Logon() {
}

std::string Logon::Encode() {
  if (NULL == field_map_.get(TAG_ENCRYPT_METHOD)) {
    field_map_.set(TAG_ENCRYPT_METHOD, 0);
  }
  if (NULL == field_map_.get(TAG_VERSION)) {
    field_map_.set(TAG_VERSION, version_);
  }
  return field_map_.Encode();
}

} /* namespace stock_logic */
