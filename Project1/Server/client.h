#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <bits/stdc++.h> 
using namespace std;


#define MAXCLIENT 5
struct ClientInfo
{
	char m_szIPAddress[16]; // IP
    int m_nPort;  // Port
	
	//uint32_t m_uiProgram;;
	//uint32_t m_uiVersion;
	list<string> filter; //subscription
};

//List of Servers IP address and port
extern list<ClientInfo> g_clientList;


