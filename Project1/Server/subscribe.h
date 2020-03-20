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


struct SubscribeInfo
{
	char s_IPAddress[16];
	int s_Port;
	string filter[4];
};

extern list<SubscribeInfo> g_subscribeList;

