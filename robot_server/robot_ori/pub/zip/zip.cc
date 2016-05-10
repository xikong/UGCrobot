#include "zip.h"

MZip::MZip(void)
: zlibStream_(NULL)
, ulTotalZipLen_(0)
, bCompress_(true)
{
}

MZip::~MZip(void)
{
	if(zlibStream_)
	{
		deflateEnd(zlibStream_);
		delete zlibStream_;
		zlibStream_ = NULL;
	}
}

unsigned long MZip::ZipData(const unsigned char* pUnZipData, unsigned long ulUnZipLen, 
							unsigned char** ppZipData)
{
	if(pUnZipData == NULL || ulUnZipLen == 0)
	{
		return 0;
	}

	unsigned long ulZipLen = 0;
	do{
		if(bCompress_ == false)
		{
			break;
		}
		if(ulUnZipLen < ENCODE_ZLIB_MIN_COMP_SIZE)
		{
			break;
		}
		*ppZipData = new unsigned char[ulUnZipLen + 300];
		if(*ppZipData == NULL)
		{
			break;
		}
		ulZipLen = ulUnZipLen + 300;
		int Ret = Zip(pUnZipData,ulUnZipLen,*ppZipData,ulZipLen);
		if(Ret != Z_OK)
		{
			delete []*ppZipData;
			ulZipLen = 0;
			break;
		}
		return ulZipLen;

	}while(0);

	*ppZipData = new unsigned char[ulUnZipLen];
	if(*ppZipData)
	{
		*(unsigned char*)*ppZipData = ZIP_TYPE_NONE;
		memcpy(*ppZipData,pUnZipData,ulUnZipLen);
		ulZipLen = ulUnZipLen;
	}
	return ulZipLen;
}

int MZip::Zip(const unsigned char* pUnZipData, unsigned long ulUnZipLen, unsigned char* pZipData, unsigned long& ulZipLen)
{
  	int err = Z_DATA_ERROR;
	if (zlibStream_ == NULL)
	{
		zlibStream_ = new z_stream;
		// Initialise stream values
		zlibStream_->zalloc = (alloc_func)0;
		zlibStream_->zfree = (free_func)0;
		zlibStream_->opaque = (voidpf)0;

		// Initialise the z_stream
		err = deflateInit(zlibStream_,9);
		if (err != Z_OK)
		{
			return err;
		}
		ulTotalZipLen_ = 0;
	}
	// Use whatever input is provided
	zlibStream_->next_in = const_cast<Bytef*>(pUnZipData);
	zlibStream_->avail_in = ulUnZipLen;
	zlibStream_->next_out = pZipData;
	zlibStream_->avail_out = ulZipLen;
	// Try to unzip the data
	err = deflate(zlibStream_, Z_SYNC_FLUSH);    
	if ((err == Z_OK) && (zlibStream_->avail_in == 0))
	{
		// All available input has been processed, everything ok.
		// Set the size to the amount unzipped in this call (including all recursive calls)
		ulZipLen = (zlibStream_->total_out - ulTotalZipLen_);
		ulTotalZipLen_ = zlibStream_->total_out;
	}

	if (err != Z_OK)
		ulZipLen = 0;
	return err;
}

void MZip::EnableCompress(bool bEnable)
{
	bCompress_ = bEnable;
}
