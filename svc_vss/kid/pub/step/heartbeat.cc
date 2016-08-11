/*
 * heartbeat.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include "heartbeat.h"

namespace step {

Heartbeat::Heartbeat() {
  set(TAG_MSG_TYPE, kMsgTypeHeartbeat);
}

Heartbeat::~Heartbeat() {
}

} /* namespace step */
