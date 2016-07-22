#ifndef KID_SHARE_MANAGER_INFO_
#define KID_SHARE_MANAGER_INFO_

#include "net/comm_head.h"

namespace plugin_share {

typedef struct ServerInfo {
  uint16 type;
  uint32 id;
  VerifyState verify_state;
  uint16 port;
  std::string mac;
  std::string password;
  char token[TOKEN_SIZE];
  uint32 max_tasks;
  uint32 exec_tasks;
  ServerInfo() {
    type = 0;
    id = 0;
    verify_state = VERIFY_STATE_UNVERIFIED;
    port = 0;
    memset(token, '\0', TOKEN_SIZE);
    max_tasks = 300;
    exec_tasks = 0;
  }
} ServerInfo;

typedef struct RouterInfo {
  uint32 id;
  int socket;
  VerifyState verify_state;
  uint16 send_err_count;
  RouterInfo() {
    id = 0;
    socket = 0;
    verify_state = VERIFY_STATE_UNVERIFIED;
    send_err_count = 0;
  }
} RouterInfo;
typedef std::map<int, RouterInfo *> RouterMap;

typedef struct SlbSession {
  const char *host;
  uint16 port;
  int socket;
  int send_err_count;
  bool is_effective;
  void Reset() {
    socket = 0;
    send_err_count = 0;
    is_effective = false;
  }
  SlbSession() {
    Reset();
  }
} SlbSession;
typedef std::map<int, SlbSession *> SlbSessionMap;

class ManagerInfo {

 public:
  ManagerInfo() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_lock, &attr);
  }
  void lock() {
    pthread_mutex_lock(&mutex_lock);
  }
  void unlock() {
    pthread_mutex_unlock(&mutex_lock);
  }

 public:
  ServerInfo svr_info;
  RouterMap router_map;
  SlbSessionMap slb_session_map_;
 private:
  pthread_mutex_t mutex_lock;
};
}

#endif
