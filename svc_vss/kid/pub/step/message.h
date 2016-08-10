#ifndef FIX_MESSAGE_H_
#define FIX_MESSAGE_H_

#include <string>
#include <map>

#include "basic/basictypes.h"

namespace step {

class Message {
 public:
  typedef std::map<int, std::string> FieldMap;
  typedef std::map<std::string, const int*> FieldOrderMap;

  static void Init();
  static void set_default_version(const std::string &ver);
 public:
  Message();
  virtual ~Message();

 public:
  void reset() { fields_.clear(); }
  void reset_msg_seq_num() { msg_seq_num_ = 1; }
  void set(int tag, int32 val) { set(tag, (uint64) val); }
  void set(int tag, uint64 val);
  void set(int tag, const char *val);
  void set(int tag, const std::string &val);
  const std::string* get(int tag) const;

  virtual std::string Encode();

  void set_version(const std::string version) { version_ = version; }
  std::string version() const {
    if (version_.empty()) {
      return Message::default_version_;
    } else {
      return version_;
    }
  }

 private:
  bool CheckHeaderFields();
 private:
  static FieldOrderMap field_orders_;
  static std::string default_version_;
  static const char sender_comp_id_[];  // 6 位字符
  static const char target_comp_id_[];
  static uint64 msg_seq_num_;
 private:
  std::string version_;
  FieldMap fields_;
};

}
;
// namespace fix

#endif
