/*
 * session_engine.h
 *
 *  Created on: 2016年5月24日
 *      Author: zjc
 */

#ifndef KID_PUB_SHARE_SESSION_ENGINE_H_
#define KID_PUB_SHARE_SESSION_ENGINE_H_

class Session {
 public:
   enum Type {
     SERVER,
     ROUTER,
     SLB
   };
   enum VerifyState {
       VERIFY_STATE_UNVERIFIED,
       VERIFY_STATE_SUCCESS,
       VERIFY_STATE_FAILED
   };
 private:
   Type type_;
   int32 id_;
   VerifyState verify_state_;
   std::string ip_;
   uint16 port_;
   std::string mac_;
   std::string password_;
   char token_[TOKEN_SIZE];
};

#endif /* KID_PUB_SHARE_SESSION_ENGINE_H_ */
