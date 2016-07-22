//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月15日 Author: kerry

#ifndef KID_LOGIC_UNIT_H_
#define KID_LOGIC_UNIT_H_

#include <sys/socket.h>
#include <string>
#include <time.h>
#include "basic/basictypes.h"
#include "basic/native_library.h"
#include "core/common.h"
#include "basic/basic_util.h"
#include "logic_pub_comm.h"
#include "net/packet_processing.h"
#include "net/comm_head.h"
#include "net/packet_define.h"

namespace logic {

class SomeUtils{
 public:
    static void* GetLibraryFunction(const std::string& library_name,
                                    const std::string& func_name );

    static void CreateToken(const int64 uid, const std::string& password,
                            std::string* token );

    static bool VerifyToken(const int64 uid, const std::string& password,
                            const std::string& token );

    static inline int8 StringToIntChar(const char* str ) {
        int8 intvalue = 0;
        base::BasicUtil::StringUtil::StringToCharInt(std::string(str),
                                                     &intvalue);
        return intvalue;
    }
    static inline int16 StringToIntShort(const char* str ) {
        int16 intvalue = 0;
        base::BasicUtil::StringUtil::StringToShortInt(std::string(str),
                                                      &intvalue);
        return intvalue;
    }

    static int GetAddressBySocket(const int sock, std::string &ip );

    static int GetPortBySocket(const int socket, int16 &port );

    static std::string GetLocalTime(const time_t time );

    static long GetCurrentTimeMs();

    static bool BackwardSearchStr(string &find, const string &src,
                                  const string &start, const char end );

    static bool ForwardSearchStr(string &find, const string &src,
                                 const string &start, const string &end );

    static void Trim(string &s );
};

class CoreSoUtils{
 public:
    static void InitSRV();

    static struct server* GetSRV();

    static struct server* srv;
};

class SendUtils{
 public:
    static int32 SendFull(int socket, const char* buffer, size_t bytes );
    static bool SendBytes(int socket, const void* bytes, int32 len,
                          const char* file, int32 line );
    static bool SendMessage(int socket, struct PacketHead *packet,
                            const char* file, int32 line );

    static struct threadrw_t* socket_lock_;
};
}  //  namespace logic

#define send_bytes (socket, bytes, len)\
    logic::SomeUtils::SendBytes(socket, bytes, len, __FILE__, __LINE__)\


#define send_message(socket, packet) \
    logic::SendUtils::SendMessage(socket, packet, __FILE__, __LINE__)\

#define closelockconnect(socket) \
    shutdown(socket, SHUT_RDWR);

#endif /* LOGIC_UNIT_H_ */
