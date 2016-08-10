/*
 * step_session.h
 *
 *  Created on: 2016年8月3日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_STEP_SESSION_H_
#define KID_PUB_STEP_STEP_SESSION_H_

class StepSession {
 public:
  StepSession();
  ~StepSession();
 public:
  void OnConnect(int sock);
  void OnDisConnect();
  void OnReceive(int sock, const char buf[], size_t len);
 private:

};

#endif /* KID_PUB_STEP_STEP_SESSION_H_ */
