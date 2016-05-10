TARGET = testexe
ACTIVEMQ_HOME = /root/activemq-setup/activemq-cpp-library-3.9.0
all : $(TARGET)

$(TARGET): example-main.o example-main1.o
	/bin/sh $(ACTIVEMQ_HOME)/libtool  --tag=CXX   --mode=link g++ -ansi -pedantic   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -W -Wall -Wextra -Wconversion -fPIC -fstrict-aliasing -Wstrict-aliasing=2 -Wno-long-long   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -Wno-non-virtual-dtor -Wno-unused-parameter -Wno-uninitialized -I/home/vander/installer/activemq-cpp-library-3.9.0/src/main -g -O2 -pthread  -o  $(TARGET) example-main.o example-main1.o $(ACTIVEMQ_HOME)/src/main/libactivemq-cpp.la  /usr/local/apr//lib/libapr-1.la -lrt -lcrypt  -lpthread     -lssl -lcrypto -L/usr/local/openssl/lib -lpthread 
	
example-main.o: test.cc activemq_client.h 
	g++ -DHAVE_CONFIG_H -I. -I/$(ACTIVEMQ_HOME)   -ansi -pedantic   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -W -Wall -Wextra -Wconversion -fPIC -fstrict-aliasing -Wstrict-aliasing=2 -Wno-long-long   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -Wno-non-virtual-dtor -Wno-unused-parameter -Wno-uninitialized -I$(ACTIVEMQ_HOME)/src/main -g -O2 -pthread -MT example-main.o -MD -MP -MF ./example-main.Tpo -c -o example-main.o `test -f 'test.cc' || echo './'`test.cc 

example-main1.o: activemq_client.cc activemq_client.h 
	g++ -DHAVE_CONFIG_H -I. -I/$(ACTIVEMQ_HOME)   -ansi -pedantic   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -W -Wall -Wextra -Wconversion -fPIC -fstrict-aliasing -Wstrict-aliasing=2 -Wno-long-long   -DLINUX -D_REENTRANT -D_GNU_SOURCE  -I/usr/local/apr//include/apr-1   -I/usr/local/openssl/include -Wno-non-virtual-dtor -Wno-unused-parameter -Wno-uninitialized -I$(ACTIVEMQ_HOME)/src/main -g -O2 -pthread -MT example-main.o -MD -MP -MF ./example-main1.Tpo -c -o example-main1.o `test -f 'activemq_client.cc' || echo './'`activemq_client.cc 


clean:
	rm -f *.o
	rm -f *.Tpo
	rm -f $(TARGET)
	




