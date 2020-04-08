OBJS = GroupServer.out BB_system.o stringTokenizer.o server.o

all : GroupServer
	
GroupServer : BB_system.o stringTokenizer.o server.o
	g++ -g -o GroupServer.out BB_system.o stringTokenizer.o server.o -lpthread

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

BB_system.o : BB_system.cpp
	g++ -g -c -o BB_system.o BB_system.cpp

server.o : Server.cpp
	g++ -g -c -o server.o Server.cpp



clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)
