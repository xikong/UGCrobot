#ifndef MIG_FM_PUBLIC_BASIC_ZIP_H___
#define MIG_FM_PUBLIC_BASIC_ZIP_H___
// Minimum zlib rectangle size in bytes.  Anything smaller will
// not compress well due to overhead.
#define ENCODE_ZLIB_MIN_COMP_SIZE (17)
typedef unsigned char       BYTE;
#include "zlib.h"
#include "basictypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	ZIP_TYPE_NONE = 0,
	ZIP_TYPE_ZLIB
};

class MZip
{
public:
	MZip(void);
	virtual ~MZip(void);

	unsigned long ZipData(const unsigned char* pUnZipData, unsigned long ulUnZipLen, unsigned char** ppZipData);
	void EnableCompress(bool bEnable);
	int Zip(const unsigned char* pUnZipData, unsigned long ulUnZipLen, unsigned char* pZipData, unsigned long& ulZipLen);

public:
	z_stream*         zlibStream_;
	unsigned long     ulTotalZipLen_;
	bool		      bCompress_;
};
#endif
