CFLAGS= -I../../../library/public	\
		-I../../../library/base		\
		-I../../					\
		-I..						\
		-D_DEBUG					\
		-ggdb3						\
		-std=c++0x	
OBJS = field_map.o header.o logon.o logout.o decoder.o session.o realtime_session.o
CXX=g++

all: $(OBJS)
	g++ $(CFLAGS) test.cc $(OBJS) -lglog -lboost_date_time
	
.cc.o:
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o a.out

