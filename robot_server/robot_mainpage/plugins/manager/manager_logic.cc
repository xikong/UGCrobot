//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#include  <string>

#include "manager_db.h"
#include "core/common.h"
#include "basic/native_library.h"
#include "net/errno.h"
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "algorithm/tea.h"
#include "manager_logic.h"

#define DEFAULT_CONFIG_PATH     "./plugins/manager/manager_config.xml"

#define TIMER_MANAGER_START		    10000
#define TIMER_HEART				        10001
#define TIMER_SEND_SELF_STATE	    10002
#define TIMER_CHECK_IS_REGISTERED 10003

#define	ROUTER_REG_SUCCESS  1
#define	ROUTER_REG_FAILED   0

namespace manager_logic {

using namespace plugin_share;

Managerlogic* Managerlogic::instance_ = NULL;

Managerlogic::Managerlogic()
    : session_mgr_(NULL) {
  if (!Init())
    assert(0);
}

Managerlogic::~Managerlogic() {
  if (NULL != session_mgr_)
    delete session_mgr_;
  session_mgr_ = NULL;
}

bool Managerlogic::Init() {
  bool r = false;
  std::string path = DEFAULT_CONFIG_PATH;
  config_ = config::FileConfig::GetFileConfig();
  if (config_ == NULL)
    return false;
  r = config_->LoadConfig(path);

  // 获取 server 相关配置
  server_type_ = atoi(config_->server_type_.c_str());
  session_mgr_ = SessionEngine::GetSessionManager();
  LOG_DEBUG2("session_mgr addr: %p", session_mgr_);
  ServerSession *srv_session = session_mgr_->GetServer();
  srv_session->set_server_type(server_type_);
  srv_session->set_mac(config_->mac_);
  srv_session->set_password(config_->password_);

  // 获取  slb 相关配置
  Session *slb = session_mgr_->GetSLB();
  slb->set_ip(config_->host_);
  slb->set_port(atoi(config_->port_.c_str()));

  return r;
}

bool Managerlogic::Startup() {
  // 该函数本应该在 构造函数中调用, 但是构造函数中获取不到 server
  // 因此把该函数放在定时任务中, 而且应该保证该函数应该在其它插件的定时任务之前执行
  // 以便其它插件能正确获取到共享数据, 但是无法保证该定时任务在其它插件的定时任务之前执行
  // 因此应该在其它插件中定时检查是否成功获取到了共享数据
  struct server* pserver = logic::CoreSoUtils::GetSRV();
  assert(pserver);

  void **p_share = pserver->get_plugin_share_data(pserver, "manager");
  assert(p_share);

  *((SessionManager**) (p_share)) = session_mgr_;

  session_mgr_->SetServerAddr(pserver->srv_conf.bindhost->ptr,
                              atoi(pserver->srv_conf.port->ptr));

  assert(pserver->create_connect_socket);
  Session::BindConnectCallback(pserver->create_connect_socket);
  if (session_mgr_->ConnectToSLB()) {
    RegSelf();
  }
  return true;
}

bool Managerlogic::RegSelf() {
  struct RegisterServer register_server;
  ServerSession *srv_session = session_mgr_->GetServer();
  MAKE_HEAD(register_server, REGISTER_SERVER, 0, 0, 0, 0, 0,
            srv_session->server_type(), 0, 0);
  register_server.level = srv_session->server_type();
  memset(register_server.password, '\0', PASSWORD_SIZE);
  memcpy(register_server.password, srv_session->password().c_str(),
         srv_session->password().size());
  memset(register_server.mac, '\0', MAC_SIZE);
  memcpy(register_server.mac, srv_session->mac().c_str(),
         srv_session->mac().size());
  register_server.port = srv_session->port();

  return session_mgr_->SendDataToSLB(&register_server);
}

bool Managerlogic::CheckIsRegistered() {
  if (!session_mgr_->SLBIsConnected()) {
    session_mgr_->ConnectToSLB();
    LOG_MSG("connection with slb is break, reconnect");
  }

  if (!session_mgr_->ServerIsValid() || !session_mgr_->ServerHasReRegisterSuccess()) {
    static int n = 0;
    LOG_MSG2("server has not register success"
        "try to register for the %d time", ++n);
    RegSelf();
    return false;
  }
  return true;
}

bool Managerlogic::SendHeart() {
  if (!session_mgr_->ServerIsValid()) {
    LOG_MSG("server has not register success, don't send heart");
    return false;
  }
  bool ret = true;
  struct PacketHead heartbeat_packet;
  ServerSession *srv_session = session_mgr_->GetServer();
  MAKE_HEAD(heartbeat_packet, HEART_TO_SLB, 0, 0, 0, 0, 0,
            srv_session->server_type(), srv_session->id(), 0);
  LOG_DEBUG("send heart");

  // SLB 心跳包
  if (session_mgr_->ServerHasReRegisterSuccess()) {
    session_mgr_->SendDataToSLB(&heartbeat_packet);
  }

  // router 心跳包
  return session_mgr_->SendDataToRouter(&heartbeat_packet);
}

bool Managerlogic::SendSelfState() {
  if (!session_mgr_->ServerIsValid()) {
    LOG_MSG("server has not register success, don't send state");
    return false;
  }
  ServerSession *srv_session = session_mgr_->GetServer();
  bool ret = true;
  struct ReplyPlgSvcTaskState task_state;
  MAKE_HEAD(task_state, PLUGIN_SVC_AVAILABLE_RESOURCE_NUM, 0, 0, 0, 0, 0,
            srv_session->server_type(), srv_session->id(), 0);
  session_mgr_->SendDataToRouter(&task_state);
  return ret;
}

Managerlogic* Managerlogic::GetInstance() {
  if (instance_ == NULL)
    instance_ = new Managerlogic();
  return instance_;
}

void Managerlogic::FreeInstance() {
  delete instance_;
  instance_ = NULL;
}

bool Managerlogic::HandleAllConnect(const char *func, struct server *srv,
                                    const int socket) {
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_MSG2("[%s] new connect, socket = %d, addr = %s", func, socket, addr.c_str());
  return true;
}

bool Managerlogic::OnManagerConnect(struct server *srv, const int socket) {
  return HandleAllConnect(__FUNCTION__, srv, socket);
}

bool Managerlogic::OnManagerMessage(struct server *srv, const int socket,
                                    const void *msg, const int len) {
  if (!session_mgr_->ServerIsValid()) {
    LOG_MSG("server has not registered success, don't process any request");
    return false;
  }
  return HandleAllMessage(__FUNCTION__, srv, socket, msg, len);
}

bool Managerlogic::HandleAllClose(const char *func, struct server *srv,
                                  const int socket) {
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_DEBUG2("[%s] connection closed, socket = %d, addr = %s", func, socket, addr.c_str());
  session_mgr_->OnClose(socket);
  return true;
}

bool Managerlogic::OnManagerClose(struct server *srv, const int socket) {
  return HandleAllClose(__FUNCTION__, srv, socket);
}

bool Managerlogic::OnBroadcastConnect(struct server *srv, const int socket,
                                      const void *msg, const int len) {
  // 在此处发送数据会失败，原因还未确定
//	std::string ip;
//	uint16 port;
//	if (logic::SomeUtils::GetAddressBySocket(socket, ip, port)) {
//	  SLBSession *slb = session_mgr_->GetSLB();
//		if (port == slb->port()) {
//		  // 该函数在 Connect 函数返回之前调用, 因此 socket 还赋值
//		  slb->set_socket(socket);
//		  slb->SetConnected();
//			if (!RegSelf()) {
//			  LOG_MSG("register self fail");
//			}
//		}
//	}
  return HandleAllConnect(__FUNCTION__, srv, socket);
}

bool Managerlogic::OnBroadcastMessage(struct server *srv, const int socket,
                                      const void *msg, const int len) {
  bool r = false;
  struct PacketHead* packet = NULL;
  if (srv == NULL || socket < 0 || msg == NULL || len < PACKET_HEAD_LENGTH)
    return false;

  if (!net::PacketProsess::UnpackStream(msg, len, &packet)) {
    LOG_ERROR2("UnpackStream Error socket %d", socket);
    net::PacketProsess::HexEncode(msg, len);
    return false;
  }
  assert(packet);
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_DEBUG2("packet->operate_code=%d, socket = %d, addr = %s",
      (int)packet->operate_code, socket, addr.c_str());
  switch (packet->operate_code) {
    case PLUGIN_SVC_REG_STATE: {
      OnSelfRegState(srv, socket, packet);
      break;
    }
    default:
      break;
  }
  net::PacketProsess::DeletePacket(msg, len, packet);
  return HandleAllMessage(__FUNCTION__, srv, socket, msg, len);
}

bool Managerlogic::OnBroadcastClose(struct server *srv, const int socket) {
  return HandleAllClose(__FUNCTION__, srv, socket);
}

bool Managerlogic::OnIniTimer(struct server *srv) {
  if (srv->add_time_task != NULL) {
    srv->add_time_task(srv, "manager", TIMER_MANAGER_START, 1, 1);
    srv->add_time_task(srv, "manager", TIMER_HEART, 10, -1);
    srv->add_time_task(srv, "manager", TIMER_SEND_SELF_STATE, 10, -1);
    srv->add_time_task(srv, "manager", TIMER_CHECK_IS_REGISTERED, 40, -1);
  }
  return true;
}

bool Managerlogic::OnTimeout(struct server *srv, char *id, int opcode,
                             int time) {
  switch (opcode) {
    case TIMER_MANAGER_START:
      Startup();
      break;
    case TIMER_HEART:
      SendHeart();
      break;
    case TIMER_SEND_SELF_STATE:
      SendSelfState();
      break;
    case TIMER_CHECK_IS_REGISTERED:
      CheckIsRegistered();
      break;
    default:
      break;
  }
  return true;
}

bool Managerlogic::OnSelfRegState(struct server* srv, int socket,
                                  struct PacketHead *packet, const void *msg,
                                  int32 len) {
  bool ret = true;
  struct PluginSvcRegState *reg_state = (struct PluginSvcRegState *) packet;
  std::string token(reg_state->token);
  LOG_DEBUG2("server register result: id: %d, token: %s",
             reg_state->server_id, token.c_str());
  ServerSession *srv_session = session_mgr_->GetServer();
  if ('\0' == reg_state->token[0]) {
    ret = false;
    if (Session::VERIFY_STATE_SUCCESS != session_mgr_->ServerVerifyState()) {
      session_mgr_->SetServerVerifyState(Session::VERIFY_STATE_FAILED);
    }
    LOG_MSG("plugin server register failed");
  } else {
    ret = true;
    // 重新注册成功
    session_mgr_->SetServerReRegisterIsSuccess(true);
    // 如果之前已经注册成功，直接返回，不重新赋值，避免不必要的加锁
    if (session_mgr_->ServerIsValid()) {
      return true;
    }
    session_mgr_->SetServerVerifyState(Session::VERIFY_STATE_SUCCESS);
    srv_session->set_id(reg_state->server_id);
    srv_session->set_token(reg_state->token, TOKEN_SIZE - 1);
  }
  return ret;
}

bool Managerlogic::HandleAllMessage(const char *func, struct server *srv,
                                    const int socket, const void *msg,
                                    const int len) {
  bool r = false;
  struct PacketHead* packet = NULL;
  if (srv == NULL || socket < 0 || msg == NULL || len < PACKET_HEAD_LENGTH)
    return false;

  if (!net::PacketProsess::UnpackStream(msg, len, &packet)) {
    LOG_ERROR2("UnpackStream Error socket %d", socket);
    net::PacketProsess::HexEncode(msg, len);
    return false;
  }
  assert(packet);
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_MSG2("func: [%s] packet->operate_code=%d, socket = %d, addr = %s",
      func, (int)packet->operate_code, socket, addr.c_str());
  switch (packet->operate_code) {
    case PLUGIN_SVC_MGR_ROUTER_REG: {
      OnRouterReg(srv, socket, packet);
      break;
    }
    case HEART_PACKET: {
      OnCheckHeartPacket(srv, socket, packet);
      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG_STATE: {
      OnRouterRegState(srv, socket, packet);
      break;
    }
    default:
      break;
  }
  net::PacketProsess::DeletePacket(msg, len, packet);
  return true;
}


bool Managerlogic::OnRouterReg(struct server* srv, int socket,
                               struct PacketHead *packet, const void *msg,
                               int32 len) {
  bool ret = true;
  struct PluginSvcMgrRouterReg *router_reg =
      (struct PluginSvcMgrRouterReg *) packet;
  std::string token(router_reg->token);
  LOG_DEBUG2("router coming, id: %d, socket: %d, token: %s",
      router_reg->router_id, socket, token.c_str());

  if (!session_mgr_->SLBIsConnected()) {
    LOG_MSG("connection with SLB is not established");
    return false;
  }

  if (!session_mgr_->ServerHasReRegisterSuccess()) {
    LOG_MSG("server has not re register success, don't process new request");
    return false;
  }

  base_logic::MLockGd lk(session_mgr_->lock_);
  RouterSession *router_session = NULL;
  // 如果 router 存在，则复用之前的 router
  if (session_mgr_->ExistRouter(router_reg->router_id)) {
    router_session = session_mgr_->GetRouterById(router_reg->router_id);
    router_session->set_verity_state(Session::VERIFY_STATE_UNVERIFIED);
    // router id 相等，但是 socket 不相等，可能是两个相同 router id 过来注册
    // 也可能是 router 断开 , 但是我没检测到
    if (!session_mgr_->ExistRouterSession(router_reg->router_id, socket)) {
      LOG_MSG2("router id %d is exist, but the socket is %d",
          router_reg->router_id, router_session->get_socket());
      router_session->set_socket(socket);
    }
  } else {
    router_session = new RouterSession();
    router_session->set_id(router_reg->router_id);
    router_session->set_socket(socket);
    // 调用该函数前必须先设置 id 和  socket
    session_mgr_->AddRouter(router_session);
  }

  router_session->set_token(token.c_str(), token.size());
  router_session->SetConnected();

  ServerSession *srv_session = session_mgr_->GetServer();
  router_reg->server_id = srv_session->id();
  router_reg->server_type = srv_session->server_type();
  if (!session_mgr_->SendDataToSLB(packet)) {
    LOG_MSG("send router register message to slb error");
    ret = false;
  }
  return ret;
}

bool Managerlogic::OnRouterRegState(struct server* srv, int socket,
                                    struct PacketHead *packet, const void *msg,
                                    int32 len) {
  bool ret = true;
  struct PluginSvcMgrRouterRegState *ret_state =
      (struct PluginSvcMgrRouterRegState *) packet;
  uint32 router_id = ret_state->router_id;
  LOG_DEBUG2("router register result: id: %d, state: %d",
      router_id, ret_state->state);

  base_logic::MLockGd lk(session_mgr_->lock_);
  RouterSession *router_session = session_mgr_->GetRouterById(router_id);
  if (NULL == router_session) {
    LOG_MSG2("don't find the router with id: %d", router_id);
    return false;
  }

  ServerSession *srv_session = session_mgr_->GetServer();
  struct ReplyRouterReg reply_router_reg;
  MAKE_HEAD(reply_router_reg, REPLY_ROUTER_REG, 0, 0, 0, 0, 0,
            srv_session->server_type(), srv_session->id(), 0);
  reply_router_reg.state = ret_state->state;
  if (!router_session->SendData(&reply_router_reg)) {
    LOG_MSG2("send router[%d] register result fail", router_id);
  }

  if (ROUTER_REG_SUCCESS == ret_state->state) {
    router_session->set_verity_state(Session::VERIFY_STATE_SUCCESS);
  } else {
    LOG_MSG2("router[%d] register fail, remove it", router_id);
    session_mgr_->DelRouterById(router_session->id());
  }
  return ret;
}

bool Managerlogic::OnGetMachineHardInfo(struct server* srv, int socket,
                                        struct PacketHead* packet,
                                        const void *msg, int32 len) {
  return true;
}

}  // namespace manager_logic

