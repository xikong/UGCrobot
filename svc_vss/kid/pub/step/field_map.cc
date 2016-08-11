#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "utils/strings.h"
#include "utils/macros.h"
#include "glog/logging.h"
#include "logout.h"
#include "step.h"

std::string escape(const std::string &s) {
  std::string ret;
  for (int i = 0; i < (int) s.size(); i++) {
    char c = s[i];
    if (c == step::STOP_BYTE) {  // SOH
      ret.push_back('|');
    } else {
      ret.push_back(c);
    }
  }
  return ret;
}

namespace step {

std::string FieldMap::default_version_ = "STEP.1.0.0";
const char FieldMap::sender_comp_id_[] = "sender";
const char FieldMap::target_comp_id_[] = "target";
uint64 FieldMap::msg_seq_num_ = 1;
FieldMap::FieldOrderMap FieldMap::field_orders_;

void FieldMap::set_default_version(const std::string &ver) {
  default_version_ = ver;
}

void FieldMap::Init() {
  field_orders_.insert(FieldOrderMap::value_type(kMsgTypeLogon, FIELD_ORDER_TAIL));
  field_orders_.insert(FieldOrderMap::value_type(kMsgTypeLogout, FIELD_ORDER_LOGOUT));
  field_orders_.insert(FieldOrderMap::value_type(kMsgTypeHeartbeat, FIELD_ORDER_HEARTBEAT));
  field_orders_.insert(FieldOrderMap::value_type(kMsgTypeResend, FIELD_ORDER_RESEND));
  Logout::Init();
}

FieldMap::FieldMap() {
}

FieldMap::~FieldMap() {
}

void FieldMap::set(int tag, uint64 val) {
  fields_.insert(TagMap::value_type(tag, str(val)));
}

void FieldMap::set(int tag, const char *val) {
  fields_.insert(TagMap::value_type(tag, str(val)));
}

void FieldMap::set(int tag, const std::string &val) {
  fields_.insert(TagMap::value_type(tag, val));
}

const std::string* FieldMap::get(int tag) const {
  TagMap::const_iterator it;
  it = fields_.find(tag);
  if (it == fields_.end()) {
    return NULL;
  }
  return &(it->second);
}

static std::string encode_field(int tag, const std::string &val) {
  std::string buffer;
  buffer.append(str(tag));
  buffer.push_back(step::SEPARATE_BYTE);
  buffer.append(val);
  buffer.push_back(step::STOP_BYTE);
  return buffer;
}

bool FieldMap::CheckHeaderFields() {
  if (NULL == get(TAG_SENDER_COMP_ID)) {
    set(TAG_SENDER_COMP_ID, sender_comp_id_);
  }
  if (NULL == get(TAG_TARGET_COMP_ID)) {
    set(TAG_TARGET_COMP_ID, target_comp_id_);
  }
  set(TAG_MSG_SEQ_NUM, msg_seq_num_++);
  std::string str_time =
      boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());
  str_time.erase(19);
  int pos = str_time.find('T');
  str_time.replace(pos,1,std::string("-"));
  str_time.replace(pos + 3,0,std::string(":"));
  str_time.replace(pos + 6,0,std::string(":"));
  set(TAG_SENDING_TIME, str_time);
  return true;
}

std::string FieldMap::Encode() {
  CheckHeaderFields();
  TagMap::const_iterator it = fields_.find(TAG_MSG_TYPE);
  if (fields_.end() == it) {
    LOG(ERROR) << "cannot find msg type";
    return std::string();
  }

  const std::string& msg_type = it->second;
  FieldOrderMap::const_iterator order_it = field_orders_.find(msg_type);
  if (field_orders_.end() == order_it) {
    LOG(ERROR) << "not support msg type: " << msg_type;
    return std::string();
  }

  std::string buffer;
  std::vector<int> fields(FIELD_ORDER_HEAD,
                          FIELD_ORDER_HEAD+arraysize(FIELD_ORDER_HEAD));
  const int* body_fileds = order_it->second;
  for (int i = 0; TAG_INVALID_FIELD != body_fileds[i]; ++i) {
    fields.push_back(body_fileds[i]);
  }
  fields.insert(fields.end(), FIELD_ORDER_TAIL,
                FIELD_ORDER_TAIL+arraysize(FIELD_ORDER_TAIL));

  for (int i = 0; i < fields.size(); ++i) {
    int tag = fields[i];
    if (TAG_BEGIN_STRING == tag
        || TAG_BODY_LENGTH == tag
        || TAG_CHECK_SUM == tag) {
      continue;
    }
    TagMap::iterator it = fields_.find(tag);
    if (fields_.end() == it) {
      continue;
    }
    const std::string& val = fields_[tag];
    buffer.append(encode_field(tag, val));
  }

  std::string header;
  header.append(encode_field(TAG_BEGIN_STRING, this->version()));
  header.append(encode_field(TAG_BODY_LENGTH, str(buffer.size())));
  buffer.insert(0, header);

  uint32 checksum = 0;
  for (int i = 0; i < buffer.size(); ++i) {
    checksum += (unsigned char)buffer[i];
  }
  checksum %= 256;
  char buf[4];
  snprintf(buf, sizeof(buf), "%03d", checksum);
  buffer.append(encode_field(TAG_CHECK_SUM, buf));

  return buffer;
}

}
// namespace fix
