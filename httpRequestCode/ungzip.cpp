#include "Request.h"

#define segment_size 2048000
int ungzip(const char* source, int len, char* des) {
	unsigned char compr[segment_size] = {0};
	unsigned char uncompr[segment_size] = {0};
	memcpy(compr, (unsigned char*)source, len);

	unsigned long comprlen = 0;
	unsigned long uncomprlen = 0;
	comprlen = len;
	uncomprlen = segment_size * 4;
	strcpy((char*)uncompr, "garbage");
	z_stream d_stream;
	memset(&d_stream, 0, sizeof(d_stream));
	int ret = inflateInit2(&d_stream, 47);
	if (ret != Z_OK) {
		printf("inflateInit2=%d\n", ret);
		return ret;
	}

	d_stream.next_in = compr;
	d_stream.avail_in = comprlen;
	int offset = 0;

	do {
		d_stream.next_out = uncompr;
		d_stream.avail_out = uncomprlen;
		
		ret = inflate(&d_stream, Z_NO_FLUSH);
		if (ret < 0) {
			printf("inflate error\n");
			return ret;
		}
		
		switch(ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				inflateEnd(&d_stream);
				printf("_____________\n");
				return ret;
		}

		int have = uncomprlen-d_stream.avail_out;
		memcpy(des+offset, uncompr, have);
		offset += have;
	}while(d_stream.avail_out == 0);

	inflateEnd(&d_stream);
	*(des+offset) = '0';
	return ret;
}

int gzdecompress(Byte *zdata, uLong nzdata, Byte *data, uLong *ndata)
{
	int err = 0;
	z_stream d_stream = {0}; /* decompression stream */
	static char dummy_head[2] = {
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};
	d_stream.zalloc = NULL;
	d_stream.zfree = NULL;
	d_stream.opaque = NULL;
	d_stream.next_in  = zdata;
	d_stream.avail_in = 0;
	d_stream.next_out = data;
	//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
	if(inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) return -1;
	//if(inflateInit2(&d_stream, 47) != Z_OK) return -1;
	while(d_stream.total_out < *ndata && d_stream.total_in < nzdata) {
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
		if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
		if(err != Z_OK) {
			if(err == Z_DATA_ERROR) {
				d_stream.next_in = (Bytef*) dummy_head;
				d_stream.avail_in = sizeof(dummy_head);
				if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) {
					return -1;
				}
			} else return -1;
		}
	}
	if(inflateEnd(&d_stream) != Z_OK) return -1;
	*ndata = d_stream.total_out;
	return 0;
}


void ungzib(std::string& compressStr, std::string& uncompressStr) { //   解压有错
	const char* encoding = strstr(compressStr.c_str(), "gzip");
	if (encoding) {
		const char* length = strstr(compressStr.c_str(), "Content-Length: ");
		unsigned long size = 0;
		char* str = NULL;
		unsigned long int len = 1;
		const char* chunked = NULL;
		if (length != NULL) {
			size = atoi(length + strlen("Content-Length: "));
			str = new char[size+1];
			memset(str, 0, size+1);
		}
		else {
			chunked = strstr(compressStr.c_str(), "chunked");
			if(chunked == NULL) {
				return;
			}
			std::cout << "gzip不确定大小..." << std::endl;
			//return;
			//gzip不确定大小
			str = new char[compressStr.size()*10];
			memset(str, 0,compressStr.size()*10);
		}
		const char* p = strstr(compressStr.c_str(), "\r\n\r\n");
		p += strlen("\r\n\r\n");
		//std::cout << ungzip(p, compressStr.size()*10, str) << std::endl;
		std::cout << gzdecompress((unsigned char*)p, compressStr.size() - (p - compressStr.c_str()), (unsigned char*)str, &len) << std::endl;
	/*	int i = uncompress((unsigned char*)str, &len, (unsigned char*)p, size);
		std::cout << "i=" << i << std::endl << "size=" << size << std::endl << "len=" << len << std::endl; // << "p=" << p << std::endl;*/
		std::cout << Z_OK << " 成功"<< std::endl << Z_MEM_ERROR << " 内存不足" << std::endl << Z_BUF_ERROR << " dest的缓冲区太小"<< std::endl << Z_DATA_ERROR <<" 输入数据有误"<<std::endl;
		
		uncompressStr = str;
		delete[] str;
		str = NULL;
	}	
}

