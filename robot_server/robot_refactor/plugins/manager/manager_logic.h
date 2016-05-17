//  Copyright (c) 2015-2015 The autocrawler Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry
#ifndef KID_MANAGER_MANAGER_LOGIC_
#define KID_MANAGER_MANAGER_LOGIC_
#include "core/common.h"
#include "manager/manager_db.h"
#include "basic/basictypes.h"
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include <pthread.h>
#include "../../pub/share/manager_info.h"

namespace manager_logic {
class Managerlogic {
public:
	Managerlogic();
	virtual ~Managerlogic();

private:
	static Managerlogic *instance_;

public:
	static Managerlogic *GetInstance();
	static void FreeInstance();
	bool OnManagerConnect(struct server *srv, const int socket);

	bool OnManagerMessage(struct server *srv, const int socket, const void *msg,
			const int len);

	bool OnManagerClose(struct server *srv, const int socket);

	bool OnBroadcastConnect(struct server *srv, const int socket,
			const void *data, const int len);

	bool OnBroadcastMessage(struct server *srv, const int socket,
			const void *msg, const int len);

	bool OnBroadcastClose(struct server *srv, const int socket);

	bool OnIniTimer(struct server *srv);

	bool OnTimeout(struct server *srv, char* id, int opcode, int time);

private:
	bool OnSelfRegState(struct server* srv, int socket,
			struct PacketHead *packet, const void *msg = NULL, int32 len = 0);

	bool OnRouterReg(struct server* srv, int socket, struct PacketHead *packet,
			const void *msg = NULL, int32 len = 0);

	bool OnRouterRegState(struct server* srv, int socket,
			struct PacketHead *packet, const void *msg = NULL, int32 len = 0);

	bool OnGetMachineHardInfo(struct server* srv, int socket,
			struct PacketHead* packet, const void *msg = NULL, int32 len = 0);

	bool OnCrawlerAvailableResourceNum(struct server* srv, int socket,
			struct PacketHead* packet, const void *msg = NULL, int32 len = 0);

	template<typename SCHDULERTYPE>
	bool OnTemplateReg(const char* mac, const char* password, const int socket,
			const int32 type, const int64 session_id);

	bool OnCheckHeartPacket(struct server* srv, int socket,
			struct PacketHead *packet, const void *msg = NULL, int32 len = 0);

	bool HandleAllConnect(const char *func, struct server *srv, const int socket);

	bool HandleAllMessage(const char *func, struct server *srv, const int socket, const void *msg,
			const int len);

	bool HandleAllClose(const char *func, struct server *srv, const int socket);
private:
	bool Init();
	bool Startup();

	bool RegSelf();
	int BuildSlbSession();
	void CloseSlbSession(int socket);
	bool SendHeart();
	bool SendSelfState();

	uint16 server_type_;
	bool  need_reg_self_;
	plugin_share::ManagerInfo *data_;
	config::FileConfig* config_;
};
}  // namespace manager_logic

#endif

