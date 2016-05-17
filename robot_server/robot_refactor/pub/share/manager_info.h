#ifndef KID_SHARE_MANAGER_INFO_
#define KID_SHARE_MANAGER_INFO_

#include "thread/base_thread_lock.h"
#include "net/comm_head.h"

namespace plugin_share {

class Client {
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
	void Reset() {
		id = 0;
		verify_state = VERIFY_STATE_UNVERIFIED;
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
} RouterInfo1;
typedef std::map<int, RouterInfo1 *> RouterMap;

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
		//InitThreadrw(&lock_);
		InitThreadMutex(&lock_, PTHREAD_MUTEX_RECURSIVE);
//		LOG_DEBUG2("manager info lock: %p", lock_);
	}
	~ManagerInfo() {
		//DeinitThreadrw(lock_);
		DeinitThreadMutex(lock_);
	}
public:
	//struct threadrw_t *lock_;
	struct threadmutex_t *lock_;
	ServerInfo svr_info;
	RouterMap router_map;
	SlbSessionMap slb_session_map_;
};

}

#endif
