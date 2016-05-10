target=test
lib = ./
all : $(target)

INCLUDE_PATHS = -I$(lib)

test.o:
	$(CXX)  -c $(lib)test.cc $(INCLUDE_PATHS) 
	
unzip.o:
	$(CXX)  -c $(lib)unzip.cc $(INCLUDE_PATHS) 
	
zip.o:
	$(CXX)  -c $(lib)zip.cc $(INCLUDE_PATHS) 
	
basictypes.o:
	$(CXX)  -c $(lib)basictypes.cc $(INCLUDE_PATHS) 
	
checksum.o:
	$(CXX)  -c $(lib)checksum.cc $(INCLUDE_PATHS) 
	
tea.o:
	$(CXX)  -c $(lib)tea.cc $(INCLUDE_PATHS) 
	
$(target) :test.o basictypes.o unzip.o zip.o checksum.o tea.o
	$(CXX) $^ -o $@ -lz
	
clean :
	rm -f $(target)  *.o