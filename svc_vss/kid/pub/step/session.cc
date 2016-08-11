/*
 * session.cc
 *
 *  Created on: 2016年8月11日
 *      Author: zjc
 */

#include "session.h"

namespace step {

Session::Session(const std::string& name)
    : socket_(-1),
      status_(UNCONNECTED),
      last_msg_time_(0),
      name_(name) {

}

Session::~Session() {
}

bool Session::OnConnect(int sock) {
  socket_ = sock;
  status_ = CONNECTED;
  return true;
}

bool Session::OnDisconnect() {
  socket_ = -1;
  status_ = UNCONNECTED;
  return true;
}

bool Session::OnReceive(const char* buf, size_t len) {
  last_msg_time_ = time(NULL);
  return true;
}

} /* namespace step */
