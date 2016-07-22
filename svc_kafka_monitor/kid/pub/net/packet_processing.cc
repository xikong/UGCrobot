/*
 * packet_processing.cc
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#include "net/packet_processing.h"
#include <list>
#include <string>
#include "basic/unzip.h"
#include "basic/zip.h"
#include "protocol/data_packet.h"
#include "net/comm_head.h"
#include "logic/logic_comm.h"
#include "net/packet_define.h"

namespace net {

void PacketProsess::UnpackOpcode(int32 &opcode, const void *packet_stream,
                                 const int len ) {

    packet::DataInPacket in_packet(reinterpret_cast<const char*>(packet_stream),
                                   len);

    int16 packet_length = in_packet.Read16();
    int8 is_zip_encrypt = in_packet.Read8();

    char* body_stream = NULL;
    int32 body_length = 0;
    int32 temp = 0;

    // 是否解压解码
    if (is_zip_encrypt == ZIP_AND_NOENCRYPT) {
        char* temp_body_stream =
                reinterpret_cast<char*>(const_cast<void*>(packet_stream))
                        + sizeof(int16) + sizeof(int8);
        const uint8* zipdata = reinterpret_cast<uint8*>(temp_body_stream);
        uint8* unzipdata = NULL;
        int32 temp_body_len = len - sizeof(int16) - sizeof(int8);
        body_length = net::PacketProsess::DecompressionStream(zipdata,
                                                              temp_body_len,
                                                              &unzipdata);
        body_stream = reinterpret_cast<char*>(unzipdata);
    } else {
        body_stream = reinterpret_cast<char*>(const_cast<void*>(packet_stream))
                + sizeof(int16) + sizeof(int8);
        body_length = len - sizeof(int16) - sizeof(int8);
    }

    packet::DataInPacket in(reinterpret_cast<char*>(body_stream), body_length);
    in.Read8();
    in.Read16();

    opcode = in.Read16();

    LOG_MSG2("Receive Server Msg, msg_len = %d, Opcode = %d \n", packet_length, opcode);
}

void PacketProsess::IsZipPacket(const bool is_zip_encrypt,
                                const int16 packet_length,
                                packet::DataOutPacket *out,
                                void **packet_stream,
                                int32 &packet_stream_length ) {

    char* packet = NULL;
    int32 packet_body_len = 0;

    char *body_stream = const_cast<char*>(out->GetData());
    if (is_zip_encrypt == ZIP_AND_NOENCRYPT) {
        uint8* zip_data = NULL;
        uint64 zip_len = 0;
        const uint8* unzip =
                const_cast<const uint8*>(reinterpret_cast<uint8*>(body_stream));
        zip_len = CompressionStream(
                unzip, packet_length - sizeof(int16) - sizeof(int8), &zip_data);
        free(body_stream);
        body_stream = NULL;
        packet = reinterpret_cast<char*>(zip_data);
        packet_body_len = zip_len;
    } else {
        packet = body_stream;
        packet_body_len = packet_length - sizeof(int16) - sizeof(int8);
    }

    packet::DataOutPacket out_packet(
            false, packet_body_len + sizeof(int16) + sizeof(int8));
    out_packet.Write16(packet_body_len + sizeof(int16) + sizeof(int8));
    out_packet.Write8(is_zip_encrypt);
    out_packet.WriteData(packet, packet_body_len);
    free(packet);
    packet = NULL;
    *packet_stream = reinterpret_cast<void*>(const_cast<char*>(out_packet
            .GetData()));
    packet_stream_length = packet_body_len + sizeof(int16) + sizeof(int8);

}

void PacketProsess::HexEncode(const void *bytes, size_t size ) {

#if 0
    struct PacketHeadHex* head = ( struct PacketHeadHex *)bytes;
    static const char kHexChars[] = "0123456789ABCDEF";
    std::string sret(size*3, '\0');
    for (size_t i = 0; i < size; ++i) {
        char b = reinterpret_cast<const char*>(bytes)[i];
        sret[(i*3)] = kHexChars[(b>>4) & 0xf];
        sret[(i*3)+1] = kHexChars[b&0xf];
        if ((((i*3)+2+1)%24) != 0)
        sret[(i * 3) + 2] = '\40';
        else
        sret[(i * 3) + 2] = '\n';
    }
    LOG_DEBUG2("===start====\nopcode[%d]:\n%s\n====end====\n",
            head->operate_code, sret.c_str());

#endif

}

uint64 PacketProsess::CompressionStream(const uint8* unzip_data,
                                        uint64 unzip_len, uint8** zip_data ) {
    MZip zip_engine;
    unzip_len = zip_engine.ZipData(unzip_data, unzip_len, zip_data);
    return unzip_len;
}

uint64 PacketProsess::DecompressionStream(const uint8* zip_data, uint64 zip_len,
                                          uint8** unzip_data ) {
    MUnZip unzip_engine;
    zip_len = unzip_engine.UnZipData(zip_data, zip_len, unzip_data);
    return zip_len;
}

} /* namespace packet */
