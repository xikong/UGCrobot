#ifndef MIG_FM_PUBLIC_BASIC_UNZIP_H___
#define MIG_FM_PUBLIC_BASIC_UNZIP_H___
#include "zlib.h"
#include "basictypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char       BYTE;

class MUnZip
{
public:
	MUnZip(void);
	virtual ~MUnZip(void);

	unsigned long UnZipData(const unsigned char* pZipData, unsigned long ulZipLen, unsigned char** ppUnZipData);
	int UnZip(const unsigned char* pZipData, unsigned long ulZipLen, unsigned char** ppUnZipData, unsigned long* ulUnZipLen, int iRecursion=0);
public:
	z_stream * zlibStream_;
	unsigned long unZipLen_;
};

#endif
