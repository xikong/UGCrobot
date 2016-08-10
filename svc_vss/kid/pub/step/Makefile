CFLAGS= -I../../../library/public	\
		-I../../../library/base		\
		-I..						\
		-D_DEBUG					\
		-ggdb3	
OBJS = message.o logon.o logout.o decoder.o 
CXX=g++

all: $(OBJS)
	g++ $(CFLAGS) test.cc $(OBJS) -lglog -lboost_date_time
	
.cc.o:
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o a.out

