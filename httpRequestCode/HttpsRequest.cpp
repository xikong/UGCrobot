#include "Request.h"

Https::Https():ssl(NULL), ctx(NULL) {
/*
	urlConnect((*itr).c_str(), client_sockfd, "https://");
	urlConnectSSL((*itr).c_str(), client_sockfd, ssl, ctx);
	sendHttpsGetRequest((*itr).c_str(), ssl, cookie);
	readHttpsResponse(ssl, response);
*/
}

Https::~Https() {
/*
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
*/
}
#if 0
void Https::readHttpsResponse(SSL*& ssl, std::string& response) {
	response.clear();

	char* buf = new char[2048];
	memset(buf, '\0', 2048);
	int ret = 1;
	while (ret > 0) {
		ret = SSL_read(ssl, buf, 2047);
		if (ret > 0) {
			response += buf;
			memset(buf, 0, 2048);
		}
	}
	delete[] buf;
}

void Https::sendHttpsGetRequest(const char* url, SSL*& ssl, std::string& cookie) {
	std::string uri;
	std::string host;
	findURI(url, host, uri, "https://");

	std::string request = "GET " + uri + " HTTP/1.1\r\n";

	request += "Host: " + host + "\r\n";
	request += "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:37.0) Gecko/20100101 Firefox/37.0\r\n";
	request += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
	request += "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n";
//	request += "Accept-Encoding: identity\n";
	request += "Accept-Encoding: gzip\r\n";
	request += "Cookie: " + cookie + "\r\n\r\n";
	request += "Connection: keep-alive\r\n";
	request += "Accept-Charset: uft-8\r\n";

	if (SSL_write(ssl, request.c_str(), request.size()) == -1) {
		std::cout << "SSL_write error" << std::endl;
	}

	return;
}

void Https::urlConnectSSL(const char* url, int& sockfd ,SSL*& ssl, SSL_CTX*& ctx) { // 未完成
	if (strstr(url, "http://") != NULL) {  // 未处理 http://www. .com/?=https://www. .com;
		return;
	}

	SSL_library_init();   // 初始化SSL
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	ctx = SSL_CTX_new(SSLv3_client_method());// 申请SSL的通讯方式
	if (ctx == NULL) {
		std::cout << "SSL_CTX_new error" << std::endl;
		return;
	}

	ssl = SSL_new(ctx);// 创建一个新SSL
	if (ssl == NULL) {
		std::cout << "SSL_new error" << std::endl;
		return;
	}

	BIO* sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);
	if (sbio == NULL) {
		std::cout << "BIO_new_socket error" << std::endl;
		return;
	}

	SSL_set_bio(ssl, sbio, sbio);
	SSL_set_fd(ssl, sockfd);// 该函数将Socket描述符fd设置为BIO的底层IO结构

/*
	int ret = SSL_set_fd(ssl, sockfd);
	if (ret == 0) {
		std::cout << "SSL_set_fd error" << std::endl;
		return;
	}
*/

	int ret = SSL_connect(ssl);  //创建一个SSL连接 
	if (ret < 1) {
		ERR_print_errors_fp(stdout);
		std::cout << "SSL_connect" << std::endl;
		return;
	}

}
#endif
