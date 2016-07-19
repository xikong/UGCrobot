/*
 * packet_processing.h
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#ifndef PUB_PACKET_PACKET_PROCESSING_H_
#define PUB_PACKET_PACKET_PROCESSING_H_

#include <stddef.h>
#include "protocol/data_packet.h"
#include "basic/basictypes.h"
#include "net/comm_head.h"
#include "net/packet_define.h"

namespace net {

class PacketProsess{
 public:

    static void UnpackOpcode(int &opcode, const void *packet_stream,
                             const int len );

    static void IsZipPacket(const bool is_zip_encrypt,
                            const int16 packet_length,
                            packet::DataOutPacket *out, void **packet_stream,
                            int32 &packet_stream_length );

    static void HexEncode(const void* bytes, size_t size );

    static uint64 CompressionStream(const uint8* unzip_data, uint64 unzip_len,
                                    uint8** zip_data );  //  压缩

    static uint64 DecompressionStream(const uint8* zip_data, uint64 zip_len,
                                      uint8** unzip_data );  //  解压缩
};

} /* namespace packet */

#endif /* PUB_PACKET_PACKET_PROCESSING_H_ */
