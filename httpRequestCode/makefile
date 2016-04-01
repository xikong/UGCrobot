CXX= g++ $(CCFLAGS)  -g
CCFLAGS= -std=c++11
LIBS= -lz -lssl -lcrypto -lpthread -lcurl -ljsoncpp librdkafka.a
obj=HttpRequest.o HttpsRequest.o ungzip.o main.o HttpSMTP.o
socket:$(obj)
		$(CXX) -o a.out $(obj) $(LIBS)

ungzip.o:Request.h
HttpRequest.o:Request.h
HttpsRequest.o:Request.h
main.o:Request.h
HttpSMTP.o:Request.h

.PHONY:clean
clean:
		-rm $(obj) *~ .*.swp
