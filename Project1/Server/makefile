OBJS = group_server stringTokenizer.o communicate_server.o communicate_xdr.o communicate_svc.o  

all : group_server
	
group_server : stringTokenizer.o communicate_server.o communicate_xdr.o communicate_svc.o
	g++ -g -o group_server stringTokenizer.o communicate_server.o communicate_xdr.o communicate_svc.o -lpthread -lnsl

communicate_xdr.o : communicate_xdr.c
	g++ -g -c -o communicate_xdr.o communicate_xdr.c

communicate_server.o : communicate_server.c
	g++ -g -c -o communicate_server.o communicate_server.c

communicate_svc.o : communicate_svc.c
	g++ -g -c -o communicate_svc.o communicate_svc.c

stringTokenizer.o : stringTokenizer.cpp
	g++ -g -c -o stringTokenizer.o stringTokenizer.cpp

#group_server.o : group_server.cpp
#	g++ -g -c -o group_server.o group_server.cpp




clean : 
	rm -rf $(OBJS)

new : 
	$(MAKE) clean
	$(MAKE)