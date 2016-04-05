#include "Request.h"

HttpSMTP::HttpSMTP(const char* host): Http(), sockfd(0), hostid(host) {
	socketInit(sockfd);
	urlConnect(hostid.c_str(), sockfd, "http://", 25);
	sendHttpSMTPRequest(sockfd, "17701600661abc", "724322240lilei@sina.com", "724322240@qq.com", "123123123123123123123", "234");
	//sendHttpSMTPRequest(sockfd, "17701600661abc", "724322240lilei@sina.com", "lilei@kunyan-inc.com", "123123123123123123123", "234");
	//sendHttpSMTPRequest(sockfd, "17701600661abc", "2179983221@qq.com", "lilei@kunyan-inc.com", "123123123123123123123", "234");
}

HttpSMTP::~HttpSMTP() {
	close(sockfd);
}

void HttpSMTP::sendHttpSMTPRequest(int& sockfd, const char* passwd, const char* sendmail, const char* recvmail, const char* data, const char* subject) {
	 /* EHLO指令是必须首先发的，相当于和服务器说hello */ 
	send(sockfd, "EHLO ...", strlen("EHLO ..."), 0);
	send(sockfd, "\r\n", strlen("\r\n"), 0);
	char* buf = new char[1024];
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: connected \n Server: " << buf << std::endl;

	/*发送 auth login 指令，告诉服务器要登录邮箱*/ 
	send(sockfd, "auth login\r\n", strlen("auth login\r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send auth login  \n Server: " << buf << std::endl;

	//把用户名发过去了
	std::string base64Name = base64Encode((const unsigned char*)sendmail, strlen(sendmail));
	send(sockfd, base64Name.c_str(), base64Name.size(), 0);
	send(sockfd, "\r\n", 2, 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send name  \n Server: " << buf << std::endl;

	// 用户密码
	std::string base64passwd = base64Encode((const unsigned char*)passwd, strlen(passwd));
	send(sockfd, base64passwd.c_str(), base64passwd.size(), 0);
	send(sockfd, "\r\n", 2, 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send passwd  \n Server: " << buf << std::endl;
	
	//发送 mail from 指令
	send(sockfd, "mail from: <", strlen("mail from: <"), 0);
	send(sockfd, sendmail, strlen(sendmail), 0);
	send(sockfd, "> \r\n", strlen("> \r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send mail  \n Server: " << buf << std::endl;

	//发送 rcpt to 指令
	send(sockfd, "rcpt to: <", strlen("rcpt to: <"), 0);
	send(sockfd, recvmail, strlen(recvmail), 0);
	send(sockfd, "> \r\n", strlen("> \r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: recv mail  \n Server: " << buf << std::endl;
	
	//主题部分
	send(sockfd, "data\r\n", strlen("data\r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send data  \n Server: " << buf << std::endl;

	send(sockfd, "from: <", strlen("from: <"), 0);
	//send(sockfd, sendmail, strlen(sendmail), 0);
	send(sockfd, "123", strlen("123"), 0);
	send(sockfd, "> \r\n", strlen("> \r\n"), 0);

	//发送主体
	send(sockfd, "subject:", strlen("subject:"), 0);
	send(sockfd, subject, strlen(subject), 0);
	send(sockfd, "\r\n\r\n", strlen("\r\n\r\n"), 0);
	send(sockfd, data, strlen(data), 0);
	send(sockfd, "\r\n\r\n.\r\n", strlen("\r\n\r\n.\r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send content  \n Server: " << buf << std::endl;
	
	send(sockfd, "quit\r\n", strlen("quit\r\n"), 0);
	memset(buf, 0, 1024);
	recv(sockfd, buf, 1024, 0);
	std::cout << "client: send quit  \n Server: " << buf << std::endl;

	delete[] buf;

}

std::string HttpSMTP::base64Encode(const unsigned char* Data,int DataByte) {
    //编码表
    const char EncodeTable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    //返回值
	std::string strEncode;
    unsigned char Tmp[4]={0};
    int LineLength=0;
    for(int i=0;i<(int)(DataByte / 3);i++)
    {
        Tmp[1] = *Data++;
        Tmp[2] = *Data++;
        Tmp[3] = *Data++;
        strEncode+= EncodeTable[Tmp[1] >> 2];
        strEncode+= EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
        strEncode+= EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
        strEncode+= EncodeTable[Tmp[3] & 0x3F];
        if(LineLength+=4,LineLength==76) {strEncode+="\r\n";LineLength=0;}
    }
    //对剩余数据进行编码
    int Mod=DataByte % 3;
    if(Mod==1)
    {
        Tmp[1] = *Data++;
        strEncode+= EncodeTable[(Tmp[1] & 0xFC) >> 2];
        strEncode+= EncodeTable[((Tmp[1] & 0x03) << 4)];
        strEncode+= "==";
    }
    else if(Mod==2)
    {
        Tmp[1] = *Data++;
        Tmp[2] = *Data++;
        strEncode+= EncodeTable[(Tmp[1] & 0xFC) >> 2];
        strEncode+= EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
        strEncode+= EncodeTable[((Tmp[2] & 0x0F) << 2)];
        strEncode+= "=";
    }
    
    return strEncode;
}

std::string HttpSMTP::base64Decode(const char* Data,int DataByte,int& OutByte)
{
    //解码表
    const char DecodeTable[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        62, // '+'
        0, 0, 0,
        63, // '/'
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
        0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
        0, 0, 0, 0, 0, 0,
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
    };
    //返回值
	std::string strDecode;
    int nValue;
    int i= 0;
    while (i < DataByte)
    {
        if (*Data != '\r' && *Data!='\n')
        {
            nValue = DecodeTable[*Data++] << 18;
            nValue += DecodeTable[*Data++] << 12;
            strDecode+=(nValue & 0x00FF0000) >> 16;
            OutByte++;
            if (*Data != '=')
            {
                nValue += DecodeTable[*Data++] << 6;
                strDecode+=(nValue & 0x0000FF00) >> 8;
                OutByte++;
                if (*Data != '=')
                {
                    nValue += DecodeTable[*Data++];
                    strDecode+=nValue & 0x000000FF;
                    OutByte++;
                }
            }
            i += 4;
        }
        else// 回车换行,跳过
        {
            Data++;
            i++;
        }
     }
    return strDecode;
}


