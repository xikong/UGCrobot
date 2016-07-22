//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry
#include  <string>
#include "core/common.h"
#include "basic/native_library.h"
#include "manager/manager_logic.h"
#include "manager/manager_db.h"
#include "net/errno.h"
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "algorithm/tea.h"

#define DEFAULT_CONFIG_PATH     "./plugins/manager/manager_config.xml"

#define TIMER_MANAGER_START		10000
#define TIMER_HEART				10001
#define TIMER_SEND_SELF_STATE	10002

#define	ROUTER_REG_SUCCESS	1
#define	ROUTER_REG_FAILED	0

namespace manager_logic {

Managerlogic* Managerlogic::instance_ = NULL;

Managerlogic::Managerlogic() :
		data_(NULL), need_reg_self_(true) {
	if (!Init())
		assert(0);
}

Managerlogic::~Managerlogic() {
	if (NULL != data_)
		delete data_;
	data_ = NULL;
}

bool Managerlogic::Init() {
	bool r = false;
	std::string path = DEFAULT_CONFIG_PATH;
	config_ = config::FileConfig::GetFileConfig();
	if (config_ == NULL)
		return false;
	r = config_->LoadConfig(path);
	server_type_ = atoi(config_->server_type_.c_str());

	return r;
}

bool Managerlogic::Startup() {
	struct server* pserver = logic::CoreSoUtils::GetSRV();
	assert(pserver);

	void **p_share = pserver->get_plugin_share_data(pserver, "manager");
	if (NULL == p_share) {
		LOG_MSG("get share data fail");
		assert(p_share);
	}
	data_ = new plugin_share::ManagerInfo();
	*((plugin_share::ManagerInfo **) (p_share)) = data_;
	//(plugin_share::ManagerInfo *)(*p_share) = data_ = new ManagerInfo();
	assert(data_);

	data_->svr_info.port = atoi(pserver->srv_conf.port->ptr);

	if (!RegSelf()) {
		LOG_MSG("server register fail");
	}
	return true;
}

bool Managerlogic::RegSelf() {
	int slb_socket = -1;
	if (-1 == (slb_socket = BuildSlbSession())) {
		LOG_MSG("build session to slb failed");
		return false;
	}

	bool ret = true;

	struct PluginSvcMgrReg plg_svc_mgr_reg;
	MAKE_HEAD(plg_svc_mgr_reg, PLUGIN_SVC_MGR_REG, 0, 0, 0, 0, 0, server_type_,
			0, 0);
	plg_svc_mgr_reg.level = atoi(config_->server_type_.c_str());
	memset(plg_svc_mgr_reg.password, '\0', PASSWORD_SIZE);
	memcpy(plg_svc_mgr_reg.password, config_->password_.c_str(),
			config_->password_.length());
	memset(plg_svc_mgr_reg.mac, '\0', MAC_SIZE);
	memcpy(plg_svc_mgr_reg.mac, config_->mac_.c_str(), config_->mac_.length());
	plg_svc_mgr_reg.port = data_->svr_info.port;

	if (!send_message(slb_socket, &plg_svc_mgr_reg)) {
		ret = false;
		need_reg_self_ = true;
		LOG_MSG("send server register message to slb error, del slb session");
		CloseSlbSession(slb_socket);
	} else {
		ret = true;
		LOG_MSG2("send server register packet to slb[%d] success",
				slb_socket);
	}
	return ret;
}

int Managerlogic::BuildSlbSession() {
	base_logic::MLockGd lk(data_->lock_);
	plugin_share::SlbSessionMap &slb_map = data_->slb_session_map_;
	plugin_share::SlbSessionMap::iterator it = slb_map.begin();
	for (; slb_map.end() != it; ++it) {
		plugin_share::SlbSession *slb_session= it->second;
		if (slb_session->is_effective)
			return slb_session->socket;
	}

	int ret_sock = -1;
	struct server* pserver = NULL;
	const char *slb_port = config_->port_.c_str();
	const char *slb_host = config_->host_.c_str();
	struct sock_adapter *slb_sock_adapter = NULL;

	do {
		if (NULL == (pserver = logic::CoreSoUtils::GetSRV())) {
			LOG_MSG("get server error");
			break;
		}

		if (NULL == pserver->create_connect_socket) {
			LOG_MSG("create_connect_socket is null");
			break;
		}

		slb_sock_adapter = pserver->create_connect_socket(pserver, slb_host,
				slb_port);
		if (NULL == slb_sock_adapter) {
			LOG_MSG2("fail to connect to slb(%s:%s)", slb_host, slb_port);
			break;
		} else {
			ret_sock = slb_sock_adapter->sock;
			LOG_MSG2("connect slb(%s:%s) success, socket: %d", slb_host, slb_port, slb_sock_adapter->sock);
		}
	} while (0);

	if (-1 != ret_sock) {
		plugin_share::SlbSession *slb_session = new plugin_share::SlbSession();
		slb_session->host = slb_host;
		slb_session->port = atoi(slb_port);
		slb_session->socket = ret_sock;
		slb_session->is_effective = true;
		plugin_share::SlbSessionMap &slb_session_map = data_->slb_session_map_;
		plugin_share::SlbSessionMap::iterator it = slb_session_map.find(
				ret_sock);
		if (slb_session_map.end() != it) {
			int socket = it->first;
			LOG_MSG2("slb socket: %d has reallocate, remove old socket: %d",
					ret_sock, socket);
			delete it->second;
			slb_session_map.erase(it);
		}
		slb_session_map[ret_sock] = slb_session;
	}
	return ret_sock;
}

void Managerlogic::CloseSlbSession(int socket) {
	if (socket <= 0)
		return;
	base_logic::MLockGd lk(data_->lock_);
	// 连接断开，清除服务器验证信息
	data_->svr_info.Reset();
	plugin_share::SlbSessionMap &slb_session_map = data_->slb_session_map_;
	plugin_share::SlbSessionMap::iterator it = slb_session_map.find(socket);
	if (slb_session_map.end() != it) {
		closelockconnect(socket);
		delete it->second;
		slb_session_map.erase(it);
		LOG_MSG2("close slb session, socket = %d, slb session count: %d",
				socket, slb_session_map.size());
	} else {
		LOG_MSG2("can't find socket[%d] in slb session map", socket);
	}
}

bool Managerlogic::SendHeart() {
	if (VERIFY_STATE_SUCCESS != data_->svr_info.verify_state) {
		RegSelf();
		return false;
	}

	LOG_DEBUG2("need_reg_self_ = %d", need_reg_self_);
	if (need_reg_self_)
		RegSelf();

	bool ret = true;
	bool is_del = false;
	bool all_router_is_invalid = true;

	struct PacketHead heartbeat_packet;
	MAKE_HEAD(heartbeat_packet, HEART_TO_SLB, 0, 0, 0, 0, 0, server_type_,
			data_->svr_info.id, 0);

	base_logic::MLockGd lk(data_->lock_);
	plugin_share::RouterMap &router_map = data_->router_map;
	plugin_share::RouterMap::iterator it = router_map.begin();
	for (; it != router_map.end();) {
		is_del = false;
		plugin_share::RouterInfo *router = it->second;
		if (VERIFY_STATE_SUCCESS != router->verify_state) {
			++it;
			continue;
		}
		if (!send_message(router->socket, &heartbeat_packet)) {
			is_del = true;
			router_map.erase(it++);
			LOG_MSG2("send heartbeat to router error, id: %d, socket: %d, del router, router count: %d",
					router->id, router->socket, router_map.size());
			delete router;
		} else {
			all_router_is_invalid = false;
			LOG_DEBUG2("send heart to router(%d, %d) success at %d",
					router->id, router->socket, time(NULL));
		}
		if (!is_del)
			++it;
	}

	//给 slb 发心跳包
	int slb_sock = -1;
	if (-1 == (slb_sock = BuildSlbSession())) {
		need_reg_self_ = true;
		LOG_MSG("fail to connect to slb, can't send heart to slb");
	} else {
		if (!send_message(slb_sock, &heartbeat_packet)) {
			need_reg_self_ = true;
			LOG_MSG2("send heart to slb error, socket = %d", slb_sock);
			CloseSlbSession(slb_sock);
		} else {
			std::string addr;
			logic::SomeUtils::GetAddressBySocket(slb_sock, addr);
			LOG_DEBUG2("succeed to send heart to slb, socket = %d, addr = %s",
					slb_sock, addr.c_str());
		}
	}
	LOG_DEBUG2("slb session count: %d", data_->slb_session_map_.size()); LOG_DEBUG2("router count: %d", data_->router_map.size());
	return ret;
}

bool Managerlogic::SendSelfState() {
	bool ret = true;
	struct ReplyPlgSvcTaskState task_state;
	MAKE_HEAD(task_state, PLUGIN_SVC_AVAILABLE_RESOURCE_NUM, 0, 0, 0, 0, 0,
			server_type_, data_->svr_info.id, 0);

	base_logic::MLockGd lk(data_->lock_);
	plugin_share::RouterMap &router_map = data_->router_map;
	plugin_share::RouterMap::iterator it = router_map.begin();
	for (; it != router_map.end(); ++it) {
		plugin_share::RouterInfo *router = it->second;
		if (VERIFY_STATE_SUCCESS != router->verify_state)
			continue;
		if (!send_message(router->socket, &task_state)) {
			LOG_MSG2("send self state to router error, id: %d, socket: %d",
					router->id, router->socket);
			ret = false;
		}
	}
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

bool Managerlogic::HandleAllConnect(const char *func, struct server *srv, const int socket) {
	std::string addr;
	logic::SomeUtils::GetAddressBySocket(socket, addr);
	LOG_MSG2("func: [%s] new connect, socket = %d, addr = %s", func, socket, addr.c_str());
	return true;
}

bool Managerlogic::OnManagerConnect(struct server *srv, const int socket) {
	return HandleAllConnect(__FUNCTION__, srv, socket);
}

bool Managerlogic::HandleAllMessage(const char *func, struct server *srv, const int socket,
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

bool Managerlogic::OnManagerMessage(struct server *srv, const int socket,
		const void *msg, const int len) {
  if (NULL == data_) return false;
	if (VERIFY_STATE_SUCCESS != data_->svr_info.verify_state) {
		LOG_MSG("wait server init complete");
		return false;
	}
	return HandleAllMessage(__FUNCTION__, srv, socket, msg, len);
}

bool Managerlogic::HandleAllClose(const char *func, struct server *srv, const int socket) {
	std::string addr;
	logic::SomeUtils::GetAddressBySocket(socket, addr);
	LOG_MSG2("func: [%s] connection closed, socket = %d, addr = %s", func, socket, addr.c_str());

	base_logic::MLockGd lk(data_->lock_);
	plugin_share::SlbSessionMap &slb_map = data_->slb_session_map_;
	if (slb_map.end() != slb_map.find(socket)) {
		need_reg_self_ = true;
		CloseSlbSession(socket);
	}
	return true;
}

bool Managerlogic::OnManagerClose(struct server *srv, const int socket) {
	return HandleAllClose(__FUNCTION__, srv, socket);
}

bool Managerlogic::OnBroadcastConnect(struct server *srv, const int socket,
		const void *msg, const int len) {
//	std::string ip;
//	uint32 port;
//	base_logic::MLockGd lk(data_->lock_);
//
//	if (NULL != data_ && logic::SomeUtils::GetAddressBySocket(socket, ip, port)) {
//		uint32 slb_port = atoi(config_->port_.c_str());
//		if (port == slb_port) {
//			RegSelf();
//			LOG_MSG2("new connection to slb, socket = %d, ip = %s, port = %d, "
//					"register self", socket, ip.c_str(), port);
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
	LOG_MSG2("packet->operate_code=%d, socket = %d, addr = %s",
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
	default:
		break;
	}
	return true;
}

bool Managerlogic::OnSelfRegState(struct server* srv, int socket,
		struct PacketHead *packet, const void *msg, int32 len) {
	bool ret = true;
	struct PluginSvcRegState *reg_state = (struct PluginSvcRegState *) packet;

	base_logic::MLockGd lk(data_->lock_);
	if ('\0' == reg_state->token[0]) {
		ret = false;
		data_->svr_info.verify_state = VERIFY_STATE_FAILED;
		LOG_MSG("plugin server register failed");
	} else {
		ret = true;
		data_->svr_info.id = reg_state->server_id;
		data_->svr_info.verify_state = VERIFY_STATE_SUCCESS;
		memcpy(data_->svr_info.token, reg_state->token, TOKEN_SIZE - 1);
		need_reg_self_ = false;
		LOG_MSG2("plugin server register success, self id: %d",
				reg_state->server_id);
	}
//	CloseSlbSession(socket);
	return ret;
}

bool Managerlogic::OnRouterReg(struct server* srv, int socket,
		struct PacketHead *packet, const void *msg, int32 len) {
	int slb_socket = -1;

	base_logic::MLockGd lk(data_->lock_);
	bool ret = true;
	do {
		if (-1 == (slb_socket = BuildSlbSession())) {
			LOG_MSG("connect to slb fail, can't registe router");
			ret = false;
			break;
		}

		struct PluginSvcMgrRouterReg *router_reg =
				(struct PluginSvcMgrRouterReg *) packet;
		router_reg->server_type = server_type_;
		router_reg->server_id = data_->svr_info.id;

		LOG_MSG2("router coming, id: %d, socket: %d",
				router_reg->router_id, socket);
		if (data_->router_map.find(socket) == data_->router_map.end()) {
			plugin_share::RouterInfo *router_info =
					new plugin_share::RouterInfo();
			router_info->id = router_reg->router_id;
			router_info->socket = socket;
			data_->router_map[socket] = router_info;
			LOG_MSG2("add new router, socket = %d, router count: %d",
					socket, data_->router_map.size());
		} else {
			LOG_MSG2("router multiple registration, id: %d, socket: %d",
					router_reg->router_id, socket);
		}

		if (!send_message(slb_socket, packet)) {
			ret = false;
			need_reg_self_ = true;
			LOG_MSG2("send router register message to slb error, slb socket: %d, del slb session",
					slb_socket);
			CloseSlbSession(slb_socket);
		} else {
			ret = true;
			LOG_MSG2("succeed to send router register packet to slb, router socket = %d", socket);
		}
	} while (0);

	return ret;
}

bool Managerlogic::OnRouterRegState(struct server* srv, int socket,
		struct PacketHead *packet, const void *msg, int32 len) {
	bool ret = true;
	struct PluginSvcMgrRouterRegState *ret_state =
			(struct PluginSvcMgrRouterRegState *) packet;
	uint32 router_id = ret_state->router_id;

	struct ReplyRouterReg reply_router_reg;

	base_logic::MLockGd lk(data_->lock_);
	MAKE_HEAD(reply_router_reg, REPLY_ROUTER_REG, 0, 0, 0, 0, 0, server_type_,
			data_->svr_info.id, 0);
	reply_router_reg.state = ret_state->state;

	plugin_share::RouterInfo *router = NULL;
	plugin_share::RouterMap::iterator it = data_->router_map.begin();
	for (; it != data_->router_map.end(); ++it) {
		router = it->second;
		if (router->id == router_id)
			break;
	}

	do {
		if (data_->router_map.end() == it) {
			LOG_MSG2("can't find the router id: %d", router_id);
			ret = false;
			break;
		}

		if (!send_message(router->socket, &reply_router_reg)) {
			ret = false;
			LOG_MSG2("send router register result to router error, id: %d, socket: %d",
					router->id, router->socket);
		} else {
			LOG_MSG2("succeed to send router register result to router, id: %d, socket: %d",
					router->id, router->socket);
		}

		if (ROUTER_REG_SUCCESS == ret_state->state) {
			data_->router_map[router->socket]->verify_state =
					VERIFY_STATE_SUCCESS;
		} else {
			data_->router_map.erase(router->socket);
			LOG_MSG2("router register fail, remove it, id: %d, socket: %d, router count: %d",
					router->id, router->socket, data_->router_map.size());
			delete router;
		}
	} while (0);

//	CloseSlbSession(socket);
	return ret;
}

bool Managerlogic::OnGetMachineHardInfo(struct server* srv, int socket,
		struct PacketHead* packet, const void *msg, int32 len) {
	return true;
}

}  // namespace manager_logic

