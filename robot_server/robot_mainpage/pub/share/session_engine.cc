/*
 * session_engine.cc
 *
 *  Created on: 2016年5月24日
 *      Author: zjc
 */

#include "session_engine.h"
#include "basic/basic_util.h"
#include "logic/logic_unit.h"

namespace plugin_share {

CreateConnectFunc Session::CreateConnect = NULL;
SessionManager* SessionEngine::session_mgr_ = NULL;
SessionEngine* SessionEngine::session_engine_ = NULL;

Session::Session()
    : type_(UNKNOWN),
      id_(0),
      verify_state_(VERIFY_STATE_UNVERIFIED),
      port_(0),
      socket_(-1),
      is_connected_(false) {
}

Session::~Session() {
  // TODO Auto-generated destructor stub
//  Close();
}

bool Session::Connect() {
  LOG_MSG2("try to connect to %s: %u", ip_.c_str(), port_);
  if (IsConnected()) {
    LOG_MSG("the connection is still connected");
    return true;
  }
  if (ip_.empty() || 0 == port_) {
    return false;
  }

  struct sock_adapter *adapter = NULL;
  struct server* p_server = logic::CoreSoUtils::GetSRV();
  adapter = CreateConnect(
      p_server, ip_.c_str(),
      base::BasicUtil::StringUtil::Int64ToString(port_).c_str());
  if (NULL == adapter) {
    LOG_MSG2("connect to %s: %u fail", ip_.c_str(), port_);
    return false;
  }
  socket_ = adapter->sock;
  is_connected_ = true;
  LOG_MSG2("connect to %s: %u success, socket: %d", ip_.c_str(), port_, socket_);
  return true;
}

bool Session::SendData(struct PacketHead* packet) {
  LOG_DEBUG2("server > id[%d] addr %s: %u, socket: %d",
             id_, ip_.c_str(), port_, socket_);
  if (!IsConnected()) {
    LOG_MSG2("connection %s: %u is break", ip_.c_str(), port_);
    return false;
  }
  if (!send_message(socket_, packet)) {
    LOG_MSG2("send data to id[%d], addr %s: %u fail, socket: %d",
             id_, ip_.c_str(), port_, socket_);
    return false;
  }
  return true;
}

void Session::OnClose() {
  Close();
}

void Session::Close() {
  if (socket_ > 0) {
    LOG_MSG2("close sessoin: %s: %u, socket: %d", ip_.c_str(), port_, socket_);
    is_connected_ = false;
    closelockconnect(socket_);
    socket_ = -1;
  }
}

SessionManager::SessionManager() {
  InitThreadMutex(&lock_, PTHREAD_MUTEX_RECURSIVE);
}

SessionManager::~SessionManager() {
  DeinitThreadMutex(lock_);
}

bool SessionManager::ExistRouter(int32 id) {
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  return id_map.end() != id_map.find(id);
}

bool SessionManager::ExistRouterSession(int32 id, int socket) {
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  RouterIdMap::const_iterator it = id_map.find(id);
  return id_map.end() != it && it->second->get_socket() == socket;
}

bool SessionManager::IsValidRouter(int32 id) {
  bool r = false;
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  RouterIdMap::const_iterator it = id_map.find(id);
  return id_map.end() != it && it->second->is_valid();
}

bool SessionManager::IsValidSocket(int socket) {
  if (socket <= 0) {
    return false;
  }
  base_logic::MLockGd lk(lock_);
  bool r = slb_.get_socket() == socket && slb_.is_valid();
  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterSocketMap::const_iterator it = socket_map.find(socket);
  return r || (socket_map.end() != it && it->second->is_valid());
}

bool SessionManager::AddRouter(RouterSession *router) {
  if (NULL == router) {
    return false;
  }
  int32 new_id = router->id();
  int new_socket = router->get_socket();
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterIdMap::const_iterator id_it = id_map.find(new_id);
  RouterSocketMap::const_iterator socket_it = socket_map.find(new_socket);

  if (id_map.end() != id_it) {
    RouterSession *old_router = id_it->second;
    LOG_MSG2("router id[%d] has existed, addr: %s: %u, socket: %d",
             old_router->id(), old_router->ip().c_str(), old_router->port(),
             old_router->get_socket());
  }
  if (socket_map.end() != socket_it) {
    RouterSession *old_router = socket_it->second;
    LOG_MSG2("router socket[%d] has existed, addr: %s: %u, id: %d",
             old_router->get_socket(), old_router->ip().c_str(), old_router->port(),
             old_router->id());
  }
  if (id_map.end() != id_it || socket_map.end() != socket_it) {
    return false;
  }

  id_map[new_id] = router;
  socket_map[new_socket] = router;
  cache_.router_list.push_back(router);
  LOG_DEBUG2("add new router, id: %d, socket: %d, router count: %d",
             new_id, new_socket, cache_.router_list.size());
  return true;
}

bool SessionManager::DelRouterById(int32 id) {
  bool ret = true;
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  RouterIdMap::iterator id_it = id_map.find(id);
  if (id_map.end() == id_it) {
    LOG_MSG2("don't find the router id: %d", id);
    return false;
  }
  id_map.erase(id_it);

  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterSocketMap::iterator socket_it = socket_map.begin();
  while (socket_map.end() != socket_it) {
    if (socket_it->second->id() == id) {
      socket_map.erase(socket_it);
      break;
    }
    ++socket_it;
  }
  if (socket_map.end() == socket_it) {
    LOG_MSG2("don't find router in router_socket_map with id: %d", id);
    ret = false;
  }
  RouterList &list = cache_.router_list;
  RouterList::iterator list_it = list.begin();
  for (; list.end() != list_it; ++list_it) {
    if ((*list_it)->id() == id) {
      delete *list_it;
      list.erase(list_it);
      break;
    }
  }
  if (list.end() == list_it) {
    LOG_MSG2("don't find router in router_list with id: %d", id);
    ret = false;
  }
  LOG_DEBUG2("after delete router[%d], router count: %d",
             id, list.size());
  return ret;
}

bool SessionManager::DelRouterBySocket(int socket) {
  base_logic::MLockGd lk(lock_);
  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterSocketMap::iterator socket_it = socket_map.find(socket);
  if (socket_map.end() == socket_it) {
    LOG_MSG2("don't find the router socket: %d", socket);
    return false;
  }
  return DelRouterById(socket_it->second->id());
}

bool SessionManager::SendDataToRouter(struct PacketHead *data) {
  base_logic::MLockGd lk(lock_);
  RouterList &list = cache_.router_list;
  RouterList::iterator it = list.begin();
  for (; list.end() != it; ++it) {
    if (!(*it)->is_valid()) {
      LOG_MSG2("router has not register success, id: %d, socket: %d",
               (*it)->id(), (*it)->get_socket());
      continue;
    }
    (*it)->SendData(data);
  }
  return true;
}

RouterSession* SessionManager::GetRouterById(int32 id) {
  base_logic::MLockGd lk(lock_);
  RouterIdMap &id_map = cache_.router_id_map;
  RouterIdMap::const_iterator it = id_map.find(id);
  if (id_map.end() == it) {
    return NULL;
  }
  return it->second;
}

RouterSession* SessionManager::GetRouterBySocket(int socket) {
  base_logic::MLockGd lk(lock_);
  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterSocketMap::const_iterator it = socket_map.find(socket);
  if (socket_map.end() == it) {
    return NULL;
  }
  return it->second;
}

void SessionManager::SetServerAddr(const std::string ip, uint16 port) {
  base_logic::MLockGd lk(lock_);
  server_.set_ip(ip);
  server_.set_port(port);
}

void SessionManager::SetServerVerifyState(Session::VerifyState state) {
  base_logic::MLockGd lk(lock_);
  server_.set_verity_state(state);
}

Session::VerifyState SessionManager::ServerVerifyState() const {
  base_logic::MLockGd lk(lock_);
  return server_.verity_state();
}

bool SessionManager::ServerIsValid() const {
  base_logic::MLockGd lk(lock_);
  return server_.is_valid();
}

void SessionManager::SetServerReRegisterIsSuccess(bool is_success) {
  base_logic::MLockGd lk(lock_);
  server_.set_re_registered(is_success);
}

bool SessionManager::ServerHasReRegisterSuccess() const {
  base_logic::MLockGd lk(lock_);
  return server_.re_registered();
}

bool SessionManager::ConnectToSLB() {
  base_logic::MLockGd lk(lock_);
  return slb_.Connect();
}

bool SessionManager::SLBIsConnected() const {
  base_logic::MLockGd lk(lock_);
  return slb_.IsConnected();
}

bool SessionManager::SendDataToSLB(struct PacketHead* packet) {
  base_logic::MLockGd lk(lock_);
  return slb_.SendData(packet);
}

void SessionManager::OnClose(int socket) {
  base_logic::MLockGd lk(lock_);
  if (slb_.get_socket() == socket) {
    slb_.OnClose();
    server_.set_re_registered(false);
  }
  RouterSocketMap &socket_map = cache_.router_socket_map;
  RouterSocketMap::iterator socket_it = socket_map.find(socket);
  if (socket_map.end() == socket_it)
    return ;
  Session *router = socket_it->second;
  LOG_MSG2("router connection break, id: %d, ip: %s, port: %u, socket: ",
           router->id(), router->ip().c_str(), router->get_socket());
  router->OnClose();
  DelRouterBySocket(socket);
}
}
