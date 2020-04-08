OBJS = Client.out BB_system.o stringTokenizer.o client.o

all : Client
	
Client : BB_system.o stringTokenizer.o client.o
	g++ -g -o Client.out BB_system.o stringTokenizer.o client.o -lpthread

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

BB_system.o : BB_system.cpp
	g++ -g -c -o BB_system.o BB_system.cpp

client.o : client.cpp
	g++ -g -c -o client.o client.cpp



clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)
