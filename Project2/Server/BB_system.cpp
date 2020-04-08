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
#include <algorithm>
#include "BB_system.h"
#include "stringTokenizer.h"

using namespace std;
#define BUFLEN 8000	//Max length of buffer
#define MAX 3000 
typedef enum {UNKNOWN_COMMAND=0, REGISTERS, DEREGISTERS,REGISTERC, DEREGISTERC,GETLIST,FRESHIPS,
POST,READ,CHOOSE,REPLY} COMMAND_TYPE;

vector<SServerInfo> Server::SIPs;

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


void Sleep_random(int timeRange){
	// simulate delay
	srand((unsigned)time(NULL));
    int random = (rand() % timeRange);
	sleep(random);
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
    
    // socket create
	listenfd = UDP_bind(targetport);
    nReceivedBytes = recvfrom(listenfd, szReceivedData, BUFLEN, 0, (struct sockaddr *)&client_addr, &nClientAddr);
    if(nReceivedBytes == -1)
    {	
        perror("recvFrom failed");	
    }

    szReceivedData[nReceivedBytes] = '\0';
    string  ReceivedData(szReceivedData);
    close(listenfd);
    return ReceivedData;
}
int UDP_send(const char* target, unsigned short targetport, const char* message)
{


	// simulate delay here
	Sleep_random(2);

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
	}
	//wait least 10 sec.
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	//Set Timeout for recv call
	
	Delay(rand()%MAX); //delay a random time
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));
	if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
	{
		perror("sendto()");
	    return -1;
	}
	//cout<<"sended message to "<<target<<" "<<targetport<<endl;

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
	if ( (s=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
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
	}
	Delay(rand()%MAX); //delay a random time
	if (sendto(s, message, strlen(message) , MSG_CONFIRM , (struct sockaddr *) &si_other, slen)==-1)
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
		//cout<<"Received message"<<endl;
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

vector<SServerInfo>::iterator findIPs(const string strIPAddress, int nPort, vector<SServerInfo>& IPs)
{
	vector<SServerInfo>::iterator it;

	for(it=IPs.begin();it!=IPs.end();it++)
    {
		
		if(strIPAddress.compare(it->m_szIPAddress) == 0 && nPort == it->m_nPort)
		{
			return it;
		}
	}

	return IPs.end();
}

void removeIP(string strIPAddress,int nPortconst,  vector<SServerInfo>& IPs)
{
	vector<SServerInfo>::iterator it;

	it = findIPs(strIPAddress, nPortconst,IPs);
	
	if(it != IPs.end())
	{
		IPs.erase(it);
	}
	
}
Server:: Server(bool Coord):IsCoordinator(Coord)
{
	
	if(Coord)
	{
		string ip_address =checkip();
		printf("The ip address of this server is %s\n",ip_address.c_str());
		this->coordinatorIP = ip_address;
		
		addServer(this->coordinatorIP, SERVERPORT);   //first element is itself
	}
	
}

vector<Article>::iterator Server:: findArticlebyID( unsigned int inID)
{
	vector<Article>::iterator it;

	for(it=articles.begin();it!=articles.end();it++)
    {
		//cout<<it->ID<<endl;
		if(it->ID == inID)
		{
			return it;
		}
	}

	return articles.end();
}

void Server::addServer(string strIPAddress, int nPort)
{
	//new ip and port then add 
	if(findIPs(strIPAddress,nPort,this->SIPs) == this->SIPs.end())
	{
        SServerInfo info;
		strcpy(info.m_szIPAddress, strIPAddress.c_str());
		info.m_nPort = nPort;
		SIPs.push_back(info);
		
	}
}
void Server::addClient(string strIPAddress, int nPort)
{
	//new ip and port then add 
	if(findIPs(strIPAddress,nPort, this->CIPs) == this->CIPs.end())
	{
        SServerInfo info;
		strcpy(info.m_szIPAddress, strIPAddress.c_str());
		info.m_nPort = nPort;
		CIPs.push_back(info);
	}
}




string Server:: getServerList()
{
	vector<SServerInfo>::iterator it;
	string strServerList;
	char szInfo[32];

	for(it=SIPs.begin();it!=SIPs.end();it++)
    {
		sprintf(szInfo, "%s;%d;", it->m_szIPAddress, it->m_nPort);
		strServerList.append(szInfo);

	}
	
	if(strServerList.length() > 0)
	{
		//Remove last ;
		strServerList.erase(strServerList.length()-1,1);
	}

	return strServerList;
}

void * Server:: listen(void * pData)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
	string strIPAddress;
    string strCommand;
	string request;

    COMMAND_TYPE command_type = UNKNOWN_COMMAND;


	int nPort = 0;
	
	string strReturn;

	nClientAddr = sizeof(client_addr);

    // socket create
	listenfd = UDP_bind(SERVERPORT);

	 //Parsing Address port Info
	CStringTokenizer stringTokenizer;
	//keep listening for data
	//wait list 10 sec.
	
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	while(1)
	{
		//Set Timeout for recv call
		setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));
		nReceivedBytes = recvfrom(listenfd, szReceivedData, BUFLEN, 0, (struct sockaddr *)&client_addr, &nClientAddr);
       
		if(nReceivedBytes == -1)
		{
			
			if(*Is_live)
			{
				
				continue;
			}
			else
			{
				perror("recvFrom failed");
				exit(-1);
			}
			
		}
		
		szReceivedData[nReceivedBytes] = '\0';
		string  ReceivedData(szReceivedData);
        //Tokenizer
	    stringTokenizer.Split(szReceivedData, ";");

		//Check Command Type;
		strCommand = stringTokenizer.GetNext();

        if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "RegisterS") == 0)
		{
			command_type = REGISTERS;
		}
		else if(!this->IsCoordinator && strcasecmp(strCommand.c_str(), "FreshIPs") == 0)
		{
			command_type = FRESHIPS;
		}
		else if(strcasecmp(strCommand.c_str(), "RegisterC") == 0)
		{
			command_type = REGISTERC;
		}
		else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "LeaveS") == 0)
		{
			command_type = DEREGISTERS;
		}
        else if(strcasecmp(strCommand.c_str(), "LeaveC") == 0)
		{
			command_type = DEREGISTERC;
		}
		else if(strcasecmp(strCommand.c_str(), "GetList") == 0)
		{
			command_type = GETLIST;
		}
		else if(strcasecmp(strCommand.c_str(), "POST") == 0)
		{
			command_type = POST;
		}
		else if(strcasecmp(strCommand.c_str(), "READ") == 0)
		{
			command_type = READ;
		}
		else if(strcasecmp(strCommand.c_str(), "CHOOSE") == 0)
		{
			command_type = CHOOSE;
		}
		else if(strcasecmp(strCommand.c_str(), "REPLY") == 0)
		{
			command_type = REPLY;
		}
		else
		{
			command_type = UNKNOWN_COMMAND;
			cout << "Unknown command" << endl;
		}

		if(command_type == REGISTERS ||command_type == REGISTERC || command_type == DEREGISTERS
		|| command_type == DEREGISTERC ||command_type == GETLIST ||command_type == FRESHIPS )
		{
			strIPAddress = stringTokenizer.GetNext();
			nPort = atoi(stringTokenizer.GetNext().c_str());

			if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
			{
				
				//Handle Register Command
				if(command_type == REGISTERS)
				{
					addServer(strIPAddress, nPort);
					cout << currentDateTime() << " : Registed a new server " << strIPAddress << ":" << nPort << endl;
					// fresh other IP
					string strServerList = "FreshIPs;";
					strServerList.append(SIPs[0].m_szIPAddress).append(";");
					strServerList.append(to_string(SIPs[0].m_nPort)).append(";");

					strServerList.append(getServerList());
					vector<SServerInfo>::iterator it;
					for(it =SIPs.begin() + 1;it!=SIPs.end();it++)
					{
						UDP_send(it->m_szIPAddress, it->m_nPort,  strServerList.c_str());
					}
				}
				else if(command_type == REGISTERC)
				{
					addClient(strIPAddress.c_str(), nPort);
					cout << currentDateTime() << " : Registed a new client " << strIPAddress << ":" << nPort << endl;
				}
				else if(command_type == DEREGISTERS)
				{
					removeIP(strIPAddress, nPort,this->SIPs);
					cout << currentDateTime() << " : DeRegisted server " << strIPAddress << ":" << nPort << endl;
				}
				else if(command_type == DEREGISTERC)
				{
					removeIP(strIPAddress, nPort,this->CIPs);
					cout << currentDateTime() << " : Leave client " << strIPAddress << ":" << nPort << endl;
				}
				else if(command_type == GETLIST) //Handle GetList Command
				{
					strReturn = getServerList();
					
					cout << currentDateTime() << " : GetList  " << inet_ntoa(client_addr.sin_addr) << ":" << client_addr.sin_port << endl;

					//UDP_send(strIPAddress.c_str(), nPort,  strReturn.c_str());
					sendto(listenfd, strReturn.c_str(), strReturn.length(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
				}
				else if(command_type == FRESHIPS)//receive newest IP
				{
					cout<<" renew server IP "<<endl;
					SIPs.clear();
					string sIP;
					int sPort;
					while((sIP=stringTokenizer.GetNext())!="")
					{ 
						sPort = atoi(stringTokenizer.GetNext().c_str());
						addServer(sIP,sPort);
						cout<<sIP<<" "<<sPort<<endl;
					}
				}
			}
			else
			{
				cout << currentDateTime() << " : Invaild IP or Port number" << endl;
			}

		}
		else
		{
			strIPAddress = stringTokenizer.GetNext();
			nPort = atoi(stringTokenizer.GetNext().c_str());
			SServerInfo clientAddr;
			strcpy(clientAddr.m_szIPAddress,strIPAddress.c_str());
			clientAddr.m_nPort = nPort;
			if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
			{
				// handle request
				for(int t = 0;t<3;++t) // delete the first 3 element
				{
					size_t pos = ReceivedData.find_first_of(";");
					if(pos == string::npos)
					{
						ReceivedData = "";
					}
					else{
						ReceivedData = ReceivedData.substr(pos + 1);
						
					} 					
				}
				request = ReceivedData;
				cout<<"request is: "<<request<<endl;
				switch(command_type)
				{
					case POST:
					Post_handler(clientAddr,request);
					break;
					case READ:
					Read_handler(clientAddr);
					break;
					case CHOOSE:
					Choose_handler(clientAddr,request);
					break;
					case REPLY:
					Reply_handler(clientAddr,request);
					break;
					default:
					break;
				}
			}
			else
			{
				cout << currentDateTime() << " : Invaild IP or Port number" << endl;
			}
		}			
        
   }
   cout<<"close listen"<<endl;
   close(listenfd);
   return nullptr;
}
 
 int Server:: RegisterS(string coordinatorIP)
 {
	char message[BUFLEN];
	cout<< "start exit"<<endl;
	const char* ip_address = checkip().c_str();
	printf("The ip address of this server is %s\n",ip_address);
	sprintf(message,"RegisterS;%s;%d;", ip_address,SERVERPORT);//[“Register;IP;Port”]
	printf("Register to coordinator.......\n");
	if(!this->IsCoordinator)
	{
		this->coordinatorIP = coordinatorIP;
		return UDP_send(coordinatorIP.c_str(), SERVERPORT, message);
	}
	else
	{
		return -1;
	}
 }

void Server:: DeregisterS(bool* Is_live)
{
	char message[BUFLEN];
	const char* ip_address = checkip().c_str();
	sprintf(message,"LeaveS;%s;%d;", ip_address,SERVERPORT);//[LeaveS;IP;Port]
	if(!(this->coordinatorIP).empty())
	{
		if(this->IsCoordinator)
		{
			removeIP(this->coordinatorIP, SERVERPORT,Server::SIPs);
			
		}
		else{
			UDP_send(this->coordinatorIP.c_str(), SERVERPORT, message);
		}
		
	}
	
	return;
}


void* Server:: Command(void * pData)
{
	char szUserInput[512];
	bool * Is_live = (bool*) pData;
	while(* Is_live)
	{
		printf("\n----------------------Instructions---------------------:\n");    
		printf("exit: leave from the register server\n");
		printf("\n---------------------------------------------------:\n"); 
		cin.getline(szUserInput, 512);
		if(strcmp(szUserInput, "exit") == 0)
		{
			this->DeregisterS(Is_live);
			*Is_live = false;
			cout<<"closing this server........."<<endl;
			break;
		}
		else
		{
			system(szUserInput);
		}
		
	}
	return nullptr;
}

void*  Server:: listenThread(void * pData)
{
	mypara *pstru;
    pstru = (struct mypara *) pData;
    Server* pThis = pstru -> _this;
    pThis->listen((void *)(pstru->Is_live));
	pthread_exit(NULL);
}
void*  Server:: CommandThread(void * pData)
{
	mypara *pstru;
    pstru = (struct mypara *) pData;
    Server* pThis = pstru -> _this;
    pThis->Command((void *)(pstru->Is_live));
	pthread_exit(NULL);
}
//increasing rank
bool comp(const Article &a, const Article &b){
     return a.ID < b.ID;
}
void  Server:: sendArticle(const char* targetIP,unsigned short targetport)// for read
{
	sort(this->articles.begin(),this->articles.end(),comp);
	vector<Article>::iterator it;
	string strContentList;//("Version;Title;author;Time;content;0$")
	for(it=articles.begin();it!=articles.end();it++)
    {
		strContentList.append(it->content[0]).append(";");
		strContentList.append(std::to_string(it->hierachy)).append("$");
	}
	
	if(strContentList.length() > 0)
	{
		
		strContentList.erase(strContentList.length()-1,1);
	}
	cout<<"sending artilce:"<<strContentList<<endl;
	UDP_send(targetIP,targetport,strContentList.c_str());
	
}