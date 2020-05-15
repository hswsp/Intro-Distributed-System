#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>  // for sleep
#include <vector>
#include <iostream>


#include "xFs_system.h"
#include "stringTokenizer.h"

using namespace std;
#define MAX 3000 



// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
string currentDateTime() 
{
	time_t     now = time(0);
    struct tm  tstruct;
    char       buf[32];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
void  Delay(int  time)//time*1000 is second
{ 
	clock_t  now  = clock(); 
	while(clock() - now < time ); 
} 


int UDP_bind(unsigned short targetport)
{
    int listenfd = 0;
    int nState = 0;

    struct sockaddr_in serv_addr;

    // socket create
	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(listenfd < 0)
	{
		perror("socket error : ");
        return -1;
	}

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serv_addr.sin_port = htons(targetport); //uint16_t htons(uint16_t hostshort);

	nState = bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (nState == -1)
	{
		perror("bind error : ");
		return -1;
	}

    return listenfd;
}
string receive(unsigned short targetport)
{
    int listenfd = 0;
    int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN];
	struct sockaddr_in  client_addr;
	nClientAddr = sizeof(client_addr);
    
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;

    // socket create
	//cout << "		--* "<< checkip() << " Start to bind port: " << targetport << endl;
	listenfd = UDP_bind(targetport);

	// add timeout for receive
	setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));


    nReceivedBytes = recvfrom(listenfd, szReceivedData, BUFLEN, 0, (struct sockaddr *)&client_addr, &nClientAddr);
    if(nReceivedBytes == -1)
    {	
        perror("recvFrom failed");
		close(listenfd); // MATTERS!
		return "FAIL";
        exit(1);	
    }

    szReceivedData[nReceivedBytes] = '\0';
    string  ReceivedData(szReceivedData);
    close(listenfd);
	//cout << "		--* "<< checkip() << "Complete close port: " << targetport << endl;
    return ReceivedData;
}
int UDP_send(const char* target, unsigned short targetport, const char* message)
{
	struct sockaddr_in si_other;
	int s;
	socklen_t slen= sizeof(si_other);
	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		return -1;
	}
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(targetport);
	if (inet_aton(target , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	//wait list 5 sec.
	timeval tv;
	tv.tv_sec  = 5;
	tv.tv_usec = 0;
	//Set Timeout for recv call
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));
	Delay(rand()%MAX); //delay a random time
	if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
	{
		perror("sendto()");
	    return -1;
	}

	close(s);
	return 0;
}
string UDP_send_receive(const char* target,unsigned short targetport, const char* message)
{
	struct sockaddr_in si_other;
	int s,recv_len;
	socklen_t slen= sizeof(si_other);
	string receivstr;
	char szReceivedData[BUFLEN];
	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		return receivstr;
	}
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(targetport);
	if (inet_aton(target , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	
	Delay(rand()%MAX); //delay a random time
	if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
	{
		perror("sendto()");
	    return receivstr;
	}

	
	//try to receive some data, this is a blocking call
	recv_len = recvfrom(s, szReceivedData, BUFLEN, 0, (struct sockaddr *) &si_other, &slen);
	
	if (recv_len == -1)
	{
		perror("recvfrom()");
	}
	else{
		szReceivedData[recv_len] = '\0';
		receivstr =  szReceivedData;
	}
	close(s);
	return receivstr;
}
string checkip()
{
    char ip_address[16];
    int fd;
    struct ifreq ifr;

    /*AF_INET - to define network interface IPv4*/
    /*Creating soket for it.*/
    fd = socket(AF_INET, SOCK_DGRAM, 0);
     
    /*AF_INET - to define IPv4 Address type.*/
    ifr.ifr_addr.sa_family = AF_INET;
     
    /*eth0 - define the ifr_name - port name
    where network attached.*/
    memcpy(ifr.ifr_name, "eno1", IFNAMSIZ-1);
     
    /*Accessing network interface information by
    passing address using ioctl.*/
    ioctl(fd, SIOCGIFADDR, &ifr);
    /*closing fd*/
    close(fd);
     
    /*Extract IP Address*/
    strcpy(ip_address,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    return string(ip_address);
}

//Check the sting whether it is vaild or not.
bool isValidIP(const string ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result != 0;
}

bool isValidPort(const int nPort)
{
	if(nPort < 1024 || nPort > 65535)
	{
		return false;
	}

	return true;
}

// vector<SServerInfo>::iterator findIPs(const string strIPAddress, int nPort, vector<SServerInfo>& IPs)
// {
// 	vector<SServerInfo>::iterator it;

// 	for(it=IPs.begin();it!=IPs.end();it++)
//     {
		
// 		if(strIPAddress.compare(it->m_szIPAddress) == 0 && nPort == it->m_nPort)
// 		{
// 			return it;
// 		}
// 	}

// 	return IPs.end();
// }

// void removeIP(string strIPAddress,int nPortconst,  vector<SServerInfo>& IPs)
// {
// 	vector<SServerInfo>::iterator it;

// 	it = findIPs(strIPAddress, nPortconst,IPs);
	
// 	if(it != IPs.end())
// 	{
// 		IPs.erase(it);
// 	}
	
// }




