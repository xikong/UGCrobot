/*
 * session_engine.h
 *
 *  Created on: 2016年5月24日
 *      Author: zjc
 */

#ifndef KID_PUB_SHARE_SESSION_ENGINE_H_
#define KID_PUB_SHARE_SESSION_ENGINE_H_

#include <string>
#include <list>
#include <map>

#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "thread/base_thread_lock.h"

namespace plugin_share {

typedef struct sock_adapter* (*CreateConnectFunc)(struct server* srv,
                                                      const char* host,
                                                      const char* port);
class Session {
 public:
  enum Type {
    UNKNOWN,
    SERVER,
    ROUTER,
    SLB
  };
  enum VerifyState {
    VERIFY_STATE_UNVERIFIED,
    VERIFY_STATE_SUCCESS,
    VERIFY_STATE_FAILED
  };
 public:
  Session();
  ~Session();
 public:
  void BindConnectCallback(CreateConnectFunc callback) { CreateConnect = callback; }
  bool Connect();
  bool SendData(struct PacketHead* packet);
  void OnClose();
  void Close();
  bool CheckHeart() const;
  bool IsConnected() const { return is_connected_; }
  void SetConnected() { is_connected_ = true; }
 public:
  void set_type(Type type) { type_ = type; }
  Type type() const { return type_; }
  void set_id(int32 id) { id_ = id; }
  int32 id() const { return id_; }
  void set_verity_state(VerifyState state) { verify_state_ = state; }
  VerifyState verity_state() const { return verify_state_; }
  bool is_valid() const { return VERIFY_STATE_SUCCESS == verify_state_; }
  void set_ip(const std::string &ip) { ip_ = ip; }
  std::string ip() const { return ip_; }
  void set_port(uint16 port) { port_ = port; }
  uint16 port() const { return port_; }
  void set_socket(int socket) { socket_ = socket; }
  int get_socket() const { return socket_; }
  void set_mac(const std::string &mac) { mac_ = mac; }
  std::string mac() const { return mac_; }
  void set_password(const std::string &passwd) { password_ = passwd; }
  std::string password() const { return password_; }
  void set_token(const char token[], size_t size) { token_.assign(token, size); }
  std::string token() const { return token_; }
 public:
  static CreateConnectFunc CreateConnect;
 protected:
  Type type_;
  int32 id_;
  VerifyState verify_state_;
  std::string ip_;
  uint16 port_;
  int socket_;
  bool is_connected_;
  std::string mac_;
  std::string password_;
  std::string token_;
};

class ServerSession : public Session {
 public:
  ServerSession()
      : server_type_(0),
        payload_(300),
        busy_tasks_(0),
        re_registered_(false) {
    set_type(SERVER);
  }
  void set_server_type(int type) {
    server_type_ = type;
  }
  int server_type() const {
    return server_type_;
  }
  void set_payload(int num) { payload_ = num; }
  int payload() const { return payload_; }
  void set_busy_tasks(int num) { busy_tasks_ = num; }
  int busy_tasks() const { return busy_tasks_; }
  void set_re_registered(bool re_registered) { re_registered_ = re_registered; }
  bool re_registered() const { return re_registered_; }
 private:
  bool re_registered_;
  int server_type_;
  int payload_;
  int busy_tasks_;
};

class SLBSession : public Session {
 public:
  SLBSession() {
    set_type(SLB);
  }
};

class RouterSession : public Session {
 public:
  RouterSession() {
    set_type(ROUTER);
  }
};

class SessionManager {
 public:
  typedef std::map<int32, RouterSession*> RouterIdMap;
  typedef std::map<int, RouterSession*> RouterSocketMap;
  typedef std::list<RouterSession*> RouterList;
  struct Cache {
    RouterList router_list;
    RouterIdMap router_id_map;
    RouterSocketMap router_socket_map;
  };
 public:
  SessionManager();
  ~SessionManager();
 public:
  bool ExistRouter(int32 id);
  bool ExistRouterSession(int32 id, int socket);
  bool IsValidRouter(int32 id);
  bool IsValidSocket(int socket);

  bool AddRouter(RouterSession *router);
  bool DelRouterById(int32 id);
  bool DelRouterBySocket(int socket);

  bool SendDataToRouter(struct PacketHead *data);

  RouterSession* GetRouterById(int32 id);
  RouterSession* GetRouterBySocket(int socket);
  SLBSession* GetSLB() { return &slb_; }
  ServerSession* GetServer() { return &server_; }

  void OnClose(int socket);

public:
  struct threadmutex_t *lock_;
private:
  ServerSession server_;
  SLBSession slb_;
  Cache cache_;
};

class SessionEngine {
 private:
  static SessionManager *session_mgr_;
  static SessionEngine *session_engine_;

  SessionEngine() {}
  virtual ~SessionEngine() {}
 public:
  static SessionManager* GetSessionManager() {
    if (session_mgr_ == NULL)
      session_mgr_ = new SessionManager();
    return session_mgr_;
  }

  static SessionEngine* GetSessionEngine() {
    if (session_engine_ == NULL)
      session_engine_ = new SessionEngine();
    return session_engine_;
  }
};

}

#endif /* KID_PUB_SHARE_SESSION_ENGINE_H_ */
