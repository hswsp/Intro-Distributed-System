OBJS = Client.out xFs_system.o stringTokenizer.o client.o

all : Client
	
Client : xFs_system.o stringTokenizer.o client.o
	g++ -g -o Client.out xFs_system.o stringTokenizer.o client.o -lpthread -lssl -lcrypto

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

xFs_system.o : xFs_system.cpp
	g++ -g -c -o xFs_system.o xFs_system.cpp

client.o : client.cpp
	g++ -g -c -o client.o client.cpp



clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)
