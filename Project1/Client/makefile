OBJS = communicate_client stringTokenizer.o  communicate_xdr.o communicate_clnt.o  communicate_client.o

all : communicate_client
	
communicate_client : stringTokenizer.o communicate_xdr.o communicate_clnt.o communicate_client.o
	g++ -g -o communicate_client communicate_client.o stringTokenizer.o  communicate_xdr.o communicate_clnt.o -lpthread -lnsl

communicate_xdr.o : communicate_xdr.c
	g++ -g -c -o communicate_xdr.o communicate_xdr.c

communicate_client.o : communicate_client.cpp
	g++ -g -c -o communicate_client.o communicate_client.cpp

communicate_clnt.o : communicate_clnt.c
	g++ -g -c -o communicate_clnt.o communicate_clnt.c

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)