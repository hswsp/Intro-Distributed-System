OBJS = Server.out xFs_system.o stringTokenizer.o server.o

all : Server
	
Server : xFs_system.o stringTokenizer.o server.o
	g++ -g -o Server.out xFs_system.o stringTokenizer.o server.o -lpthread -lssl -lcrypto

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

xFs_system.o : xFs_system.cpp
	g++ -g -c -o xFs_system.o xFs_system.cpp

server.o : server.cpp
	g++ -g -c -o server.o server.cpp



clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)
