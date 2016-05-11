#include "unzip.h"

enum
{
	ZIP_TYPE_NONE = 0,
	ZIP_TYPE_ZLIB
};

#define MAX_BLOCKSIZE				184320

MUnZip::MUnZip(void)
: zlibStream_(NULL)
, unZipLen_(0)
{
}

MUnZip::~MUnZip(void)
{
	if(zlibStream_)
	{
		inflateEnd(zlibStream_);
		delete zlibStream_;
		zlibStream_ = NULL;
	}
}

unsigned long MUnZip::UnZipData(const unsigned char* pZipData, unsigned long ulZipLen, unsigned char** ppUnZipData)
{
	if(pZipData == NULL || ulZipLen == 0)
	{
		return 0;
	}
	unsigned long ulUnZipLen = 0;
	unsigned char ucType = *(unsigned char*)pZipData;

	ulUnZipLen = ulZipLen * 2;
	if(ulUnZipLen > MAX_BLOCKSIZE + 300)
	{
		ulUnZipLen = MAX_BLOCKSIZE + 300;
	}
	*ppUnZipData = new unsigned char[ulUnZipLen];
	if(*ppUnZipData)
	{
		int Ret = UnZip(pZipData,ulZipLen,ppUnZipData,&ulUnZipLen);
		if(Ret != Z_OK || ulUnZipLen == 0)
		{
			delete []*ppUnZipData;
			*ppUnZipData = NULL;
			ulUnZipLen = 0;
		}
	}
	return ulUnZipLen;
}

int MUnZip::UnZip(const unsigned char* pZipData, unsigned long ulZipLen, unsigned char** ppUnZipData, unsigned long* ulUnZipLen, int iRecursion)
{
  	int err = Z_DATA_ERROR;
	if (zlibStream_ == NULL)
	{
		zlibStream_ = new z_stream;
		// Initialise stream values
		zlibStream_->zalloc = (alloc_func)0;
		zlibStream_->zfree = (free_func)0;
		zlibStream_->opaque = (voidpf)0;

		// Set output data streams, do this here to avoid overwriting on recursive calls
		zlibStream_->next_out = (*ppUnZipData);
		zlibStream_->avail_out = (*ulUnZipLen);

		// Initialise the z_stream
		err = inflateInit(zlibStream_);
		if (err != Z_OK)
		{
			return err;
		}
		unZipLen_ = 0;
	}
	// Use whatever input is provided
	zlibStream_->next_in	 = const_cast<Bytef*>(pZipData);
	zlibStream_->avail_in = ulZipLen;

	// Only set the output if not being called recursively
	if (iRecursion == 0)
	{
		zlibStream_->next_out = (*ppUnZipData);
		zlibStream_->avail_out = (*ulUnZipLen);
	}

	// Try to unzip the data
	err = inflate(zlibStream_, Z_SYNC_FLUSH);    
	if ((err == Z_OK) && (zlibStream_->avail_out == 0) && (zlibStream_->avail_in != 0))
	{
		// Output array was not big enough, call recursively until there is enough space
		// What size should we try next
		unsigned long newLength = (*ulUnZipLen) *= 2;
		if (newLength == 0)
			newLength = ulZipLen * 2;

		// Copy any data that was successfully unzipped to new array
		BYTE *temp = new BYTE[newLength];
		memcpy(temp, (*ppUnZipData), (zlibStream_->total_out - unZipLen_));
		delete[] (*ppUnZipData);
		(*ppUnZipData) = temp;
		(*ulUnZipLen) = newLength;

		// Position stream output to correct place in new array
		zlibStream_->next_out = (*ppUnZipData) + (zlibStream_->total_out - unZipLen_);
		zlibStream_->avail_out = (*ulUnZipLen) - (zlibStream_->total_out - unZipLen_);

		// Try again
		err = UnZip(zlibStream_->next_in, zlibStream_->avail_in, ppUnZipData, ulUnZipLen, iRecursion + 1);
	}
	else if ((err == Z_OK) && (zlibStream_->avail_in == 0))
	{
		// All available input has been processed, everything ok.
		// Set the size to the amount unzipped in this call (including all recursive calls)
		(*ulUnZipLen) = (zlibStream_->total_out - unZipLen_);
		unZipLen_ = zlibStream_->total_out;
	}

	if (err != Z_OK)
		(*ulUnZipLen) = 0;
	return err;
}