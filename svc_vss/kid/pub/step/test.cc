#include <stdio.h>
#include <stdlib.h>
#include "glog/logging.h"
#include "logon.h"
#include "logout.h"
#include "step.h"

extern std::string escape(const std::string &s);

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  step::FieldMap::Init();
  step::FieldMap::set_default_version(kBeginString);
  step::Logout msg(step::Logout::kOutOfSysOpenTime, "logout test");
//  msg.set(34, 1);
//  msg.set(35, "A");
//  msg.set(49, "CLIENT06");
//  msg.set(56, "SERVER");
//  msg.set(52, "20150424-03:06:59.114");
//  msg.set(96, "H:12345678:12345678:");
//  msg.set(98, 0);
//  msg.set(141, "Y");
//  msg.set(108, 20);

  std::string s = msg.Encode();
  printf("%s \n", escape(s).c_str());
  // 将 s 写入 socket, 相当于向服务器发送了请求

  step::Decoder decoder;
  decoder.Push("test", strlen("test"));
  for (int i = 0; i < 2000; i++) {
    // 模拟从网络读取数据, 一次只读取一个字节
    decoder.Push(s.data() + (i % s.size()), 1);
    while (1) {
      step::FieldMap tmp;
      int ret = decoder.Parse(&tmp);
      if (ret == step::Decoder::SUCCESS) {
        printf("%s \n", escape(tmp.Encode()).c_str());
      } else if (ret == step::Decoder::PACKET_NOT_FULL) {
        // 报文未就绪, 继续读网络
        break;
      }

      // 继续解析, 因为可能一次读取到多个报文
    }
  }
  printf("%s\n", escape(decoder.buffer()).data());
  return 0;
}
