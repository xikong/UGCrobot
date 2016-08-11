/*
 * heartbeat.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_HEARTBEAT_H_
#define KID_PUB_STEP_HEARTBEAT_H_

#include "step.h"
#include "header.h"

namespace step {

class Heartbeat : public Header {
 public:
  Heartbeat();
  ~Heartbeat();
 public:
  static std::string MsgType() { return kMsgTypeHeartbeat; }
};

} /* namespace step */

#endif /* KID_PUB_STEP_HEARTBEAT_H_ */
