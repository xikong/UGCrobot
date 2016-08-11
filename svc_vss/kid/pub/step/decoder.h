#ifndef FIX_DECODER_H_
#define FIX_DECODER_H_

#include <string>

#include "field_map.h"

namespace step {

class Decoder {
 public:
  enum {
    SUCCESS,
    BAD_PACKET,
    PACKET_NOT_FULL,
    CHECK_SUM_FAIL
  };
  Decoder() {
    buffer_.reserve(BUFFER_SIZE);
  }
  // 当你从 socket 中读到数据, 或者从文件中读到数据时, 将数据压入解码器的数据缓冲区
  int Push(const char *buf, int len);
  std::string buffer() const { return buffer_; }
  bool LocateHead();
  uint32 CalcChecksum(const char* body_tail);
  bool IsFullStep();
  // 解析缓冲区中的数据, 如果解析出一个完整的 FIX 报文, 返回 1,
  // 如果数据不足一个报文, 返回 0, 你应该继续读取数据并压入解码器.
  // 如果出错(如系统错误), 返回 -1.
  int Parse(FieldMap *msg);
 private:
  static const size_t BUFFER_SIZE = 20 * 1024 * 1024;
  std::string buffer_;
};

}
// namespace step

#endif
