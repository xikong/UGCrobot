//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月15日 Author: kerry

#ifndef KID_LOGIC_UNIT_H_
#define KID_LOGIC_UNIT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include "basic/basictypes.h"
#include "basic/native_library.h"
#include "net/comm_head.h"
#include "../../library/core/common.h"
#include "net/packet_processing.h"
#include "logic/logic_comm.h"
#include "basic/basic_util.h"

namespace logic {

class SomeUtils {
public:
	static void* GetLibraryFunction(const std::string& library_name,
			const std::string& func_name);

	static void CreateToken(const int64 uid, const std::string& password,
			std::string* token);

	static bool VerifyToken(const int64 uid, const std::string& password,
			const std::string& token);

	static inline int8 StringToIntChar(const char* str) {
		int8 intvalue = 0;
		base::BasicUtil::StringUtil::StringToCharInt(std::string(str),
				&intvalue);
		return intvalue;
	}
	static inline int16 StringToIntShort(const char* str) {
		int16 intvalue = 0;
		base::BasicUtil::StringUtil::StringToShortInt(std::string(str),
				&intvalue);
		return intvalue;
	}
	static inline bool GetAddressBySocket(const int sock, std::string &addr) {
		bool r = true;
		std::string ip;
		uint32 port;
		char s_port[6];
		memset(s_port, '\0', sizeof(s_port));
		struct sockaddr_storage sa;
		int salen = sizeof(sa);
		if (::getpeername(sock, (struct sockaddr*) &sa, (socklen_t *) &salen)
				== -1) {
			return false;
		}

		if (sa.ss_family == AF_INET) {
			struct sockaddr_in *s = (struct sockaddr_in*) &sa;
			ip = ::inet_ntoa(s->sin_addr);
			port = ::ntohs(s->sin_port);
			sprintf(s_port, "%u", port);
			addr = ip + ": " + s_port;
		}
		return r;
	}
  static inline bool GetAddressBySocket(const int sock, std::string &ip,
                                        uint16 &port) {
    bool r = true;
    struct sockaddr_storage sa;
    int salen = sizeof(sa);
    if (::getpeername(sock, (struct sockaddr*) &sa, (socklen_t *) &salen)
        == -1) {
      return false;
    }

    if (sa.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in*) &sa;
      ip = std::string(::inet_ntoa(s->sin_addr));
      port = ::ntohs(s->sin_port);
    }
    return r;
  }
};

class CoreSoUtils {
public:
	static void InitSRV();

	static struct server* GetSRV();

	static struct server* srv;
};

class SendUtils {
public:
	static int32 SendFull(int socket, const char* buffer, size_t bytes);
	static bool SendBytes(int socket, const void* bytes, int32 len,
			const char* file, int32 line);
	static bool SendMessage(int socket, struct PacketHead* packet,
			const char* file, int32 line);
	static bool SendHeadMessage(int socket, int32 operate_code, int32 type,
			int32 is_zip_encrypt, int64 session, int32 reserved,
			const char* file, int32 line);

	static bool SendErronMessage(int socket, int32 operate_code, int32 type,
			int32 is_zip_encrypt, int64 session, int32 reserved, int32 error,
			const char* file, int32 line);

	static struct threadrw_t* socket_lock_;
};
}  //  namespace logic

#define send_bytes (socket, bytes, len)\
    logic::SomeUtils::SendBytes(socket, bytes, len, __FILE__, __LINE__)\


#define send_message(socket, packet) \
    logic::SendUtils::SendMessage(socket, packet, __FILE__, __LINE__)\


#define send_headmsg(socket, operate_code, type, is_zip_encrypt, \
        reserved, session)\
    logic::SendUtils::SendHeadMessage(socket, operate_code, type, \
            is_zip_encrypt, reserved, session, __FILE__, __LINE__)\


#define send_errnomsg(socket, operate_code, type, is_zip_encrypt, \
        reserved, session, error)\
    logic::SendUtils::SendErronMessage(socket, operate_code, type, \
            is_zip_encrypt, reserved, session, error, __FILE__, __LINE__)\

#define closelockconnect(socket) \
    shutdown(socket, SHUT_RDWR);

#endif /* LOGIC_UNIT_H_ */
