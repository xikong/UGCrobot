#include <stdio.h>
#include <iostream>
#include "zip.h"
#include "unzip.h"
#include "basictypes.h"
#include "checksum.h"
#include "tea.h"
using namespace std;
using namespace en_de;

void HexEncode(const void *bytes, size_t size) {
//  #if defined HEXDUMP
    struct PacketHead* head = (struct PacketHead*)bytes;
    static const char kHexChars[] = "0123456789ABCDEF";
    std::string sret(size*3, '\0');
    for (size_t i = 0; i < size; ++i) {
        char b = reinterpret_cast<const char*>(bytes)[i];
        sret[(i*3)] = kHexChars[(b>>4) & 0xf];
        sret[(i*3)+1] = kHexChars[b&0xf];
        if ((((i*3)+2+1)%12) != 0)
            sret[(i * 3) + 2] = '\40';
        else
            sret[(i * 3) + 2] = '\n';
    }
    printf("mem :============ \n%s\n", sret.c_str());
//  #endif
}
int main(void) {
    const unsigned char* unzipData = (const unsigned char*)"1283784816481helo!";
    char *pChar = "1283784816481helo!";
    
    unsigned char* zipData;
    unsigned char* unzip_data;
    //unsigned char[100];
    //memset(zip, '\0', sizeof(zip));
    int len = strlen(pChar);
    int zip_len, unzip_len;
    printf("orient msg: %s\n", unzipData);
    printf("orient length : %d\n", len);
    
    MZip zip_engine;
    zip_len = zip_engine.ZipData(unzipData, len, &zipData);
    printf("zip_len : %d\n", zip_len);
    printf("zipData : %s\n", zipData);
    
    MUnZip unzip_engine;
    unzip_len = unzip_engine.UnZipData(zipData, zip_len, &unzip_data);
    printf("unzip_len : %d\n", unzip_len);
    printf("unzipData : %s\n", unzip_data);
    int16_t check_ret;
    printf("checksum run ...\n");
    if (true == checksum((const char*)pChar, &check_ret)) {
        printf("check_ret is : 0x%x\n", (unsigned int)check_ret);
        printf("check_ret is : %d\n", (unsigned int)check_ret);
    } else {
        printf("checksum error!\n");
        return -1;
    }
    string sSrc = "1283784816481helo!";
    std::string sDst;
    printf("tea run ...\n");
    printf("orient str is : %s\n", sSrc.c_str());
    sDst = en_de::str_en(sSrc, 16);
    printf("encrypt str is : %s\n", sDst.c_str());
    
    HexEncode(sDst.c_str(), len);
    
    
    string deStr;
    deStr = en_de::str_de(sDst, 16);
    printf("decrypt str is : %s\n", deStr.c_str());
    
    //dayin();
    return 0;
}