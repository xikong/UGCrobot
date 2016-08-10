#include <iostream>
#include <cstring>

#include <boost/lexical_cast.hpp>

#include "glog/logging.h"
#include "utils/strings.h"
#include "fix.h"

namespace step {

int Decoder::Push(const char *buf, int len) {
  buffer_.append(buf, len);
  return len;
}

bool Decoder::LocateHead() {
  if (buffer_.size() < STEP_PACKET_MIN_SIZE) {
    return false;
  }
  std::string::size_type head_pos = buffer_.find(kStepHeaderFlag);
  if (std::string::npos == head_pos) {
    // 超过缓存大小还是没有找到 step header, 则清空缓存
    if (buffer_.size() > BUFFER_SIZE) {
      LOG(ERROR)<< "not find step header with buffer size: "
      << buffer_.size() << ", clear buffer";
      buffer_.clear();
    }
    return false;
  }

  if (0 != head_pos) {
    LOG(ERROR)<< "garbled message: " << buffer_.substr(0, head_pos);
    buffer_.erase(0, head_pos);
  }
  return buffer_.size() > STEP_PACKET_MIN_SIZE;
}

uint32 Decoder::CalcChecksum(const char* body_tail) {
  uint32 checksum = 0;
  const char* p = buffer_.data();
  while (p < body_tail) {
    checksum += (unsigned char)*p++;
  }
  return checksum % 256;
}

bool Decoder::IsFullStep() {
  while (LocateHead()) {
    int i = 1;
    int tag = 0;
    int body_len = 0;
    const char* body = NULL;
    const char* body_tail = NULL;
    int checksum = 0;
    int flag = SUCCESS;

    const char* start = buffer_.data() + sizeof(kStepHeaderFlag);
    const char* end = buffer_.data() + buffer_.size();

    while (start < end) {
      ++i;
      // key
      const char* cur_pos =
          (const char*) memchr(start, step::SEPARATE_BYTE, end - start);
      if (NULL == cur_pos) {
        return false;
      }
      int key_len = cur_pos - start;
      std::string key_s = std::string(start, key_len);
      tag = str_to_int(key_s);
      start = cur_pos + 1;

      // value
      cur_pos = (const char*) memchr(start, step::STOP_BYTE, end - start);
      if (NULL == cur_pos) {
        return false;
      }
      int val_len = cur_pos - start;
      std::string val_s = std::string(start, val_len);
      start = cur_pos + 1;

      if (2 == i) {
        // TAG_BODY_LENGTH 一定是第二个标签
        if (TAG_BODY_LENGTH != tag) {
          flag = BAD_PACKET;
          break;
        }
        body = start;
        body_len = str_to_int(val_s);
      } else if (3 == i) {
        // TAG_MSG_TYPE 一定是第三个标签
        if (TAG_MSG_TYPE != tag) {
          flag = BAD_PACKET;
          break;
        }
        body_tail = start = body + body_len;
      } else {
        // TAG_CHECK_SUM 一定是最后的标签
        if (TAG_CHECK_SUM != tag) {
          flag = BAD_PACKET;
        } else {
          checksum = str_to_int(val_s);
        }
        break;
      }
    } // while (end > start)
    if (start >= end) { // 包不完整
      return false;
    }
    if (BAD_PACKET == flag) {
      LOG(ERROR) << "bad packet, tag: " << tag;
      // 向后移动一个头的距离, 重新定位头
      buffer_.erase(0, sizeof(kStepHeaderFlag));
      continue;
    }
    int calc_checksum = CalcChecksum(body_tail);
    if (calc_checksum != checksum) {
      LOG(ERROR) << "checksum fail, checksum:" << checksum
          << ", calc checksum:" << calc_checksum;
      buffer_.erase(0, sizeof(kStepHeaderFlag));
      continue;
    }
    return true;
  } // while (LocateHead())
  return false;
}

int Decoder::Parse(Message *msg) {
  if (!IsFullStep()) {
    return PACKET_NOT_FULL;
  }

  int raw_data_len;
  const char *start = buffer_.data();
  const char* end = start + buffer_.size();

  msg->reset();
  while (end > start) {
    // key
    const char *cur_pos =
        (const char *) memchr(start, step::SEPARATE_BYTE, end - start);
    if (cur_pos == NULL) {
      break;
    }
    int key_len = cur_pos - start;
    std::string key_s(start, key_len);
    int tag = str_to_int(key_s);
    start = cur_pos + 1;

    // value
    if (raw_data_len > 0 && TAG_RAW_DATA == tag) {
      cur_pos = start + raw_data_len;
    } else {
      cur_pos = (const char *) memchr(start, step::STOP_BYTE, end - start);
      if (end == NULL) {
        break;
      }
    }
    int val_len = cur_pos - start;
    std::string val_s(start, val_len);
    msg->set(tag, val_s);
    start = cur_pos + 1;

    if (TAG_CHECK_SUM == tag) {  // checksum
      // 一个完整的报文解析结束, 从缓冲区清除已经解析了的数据
      int calc_checksum = str_to_int(val_s);
      if (start > end) start = end;
      buffer_.erase(buffer_.begin(),
                    std::string::iterator(const_cast<char*>(start)));
      return SUCCESS;
    }

    if (TAG_RAW_DATA_LENGTH == tag) {
      raw_data_len = str_to_int(val_s);
    }
  }
  return PACKET_NOT_FULL;
}

}
// namespace fix
