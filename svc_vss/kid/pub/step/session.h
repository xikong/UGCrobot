/*
 * session.h
 *
 *  Created on: 2016年8月11日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_SESSION_H_
#define KID_PUB_STEP_SESSION_H_

#include <ctime>

#include <string>

namespace step {

class Session {
 public:
  enum Status {
    UNCONNECTED,
    CONNECTING,
    CONNECTED
  };
  Session(const std::string& name);
  virtual ~Session();
 public:
  std::string name() const { return name_; }
  virtual bool OnConnect(int sock);
  virtual bool OnDisconnect();
  virtual bool OnReceive(const char* buf, size_t len);
 protected:
  int socket_;
  Status status_;
  time_t last_msg_time_;
  std::string name_;
};

} /* namespace step */

#endif /* KID_PUB_STEP_SESSION_H_ */
