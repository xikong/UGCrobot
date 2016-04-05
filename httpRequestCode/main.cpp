#include "Request.h"

int main(void) {
	signal(SIGPIPE, &signalHandler);
	Http httpRequest(5);
	//HttpSMTP("http://smtp.sina.com");
	//HttpSMTP("http://smtp.qq.com");
	return 0;
	printf("return 0------------------------------------\n");
}

void signalHandler(int) {
	std::cout << "SIGPIPE" << std::endl;
}
