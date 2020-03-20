OBJS = super_server super_server.o stringTokenizer.o

all : super_server
	
super_server : super_server.o stringTokenizer.o
	g++ -g -o super_server super_server.o stringTokenizer.o -lpthread

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

super_server.o : super_server.cpp
	g++ -g -c -o super_server.o super_server.cpp

clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)
