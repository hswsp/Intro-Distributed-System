#include <iostream>
#include <fstream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <list>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include "stringTokenizer.h"

#define PORT 5105

typedef enum {UNKNOWN_FORMAT=0, RPC, RMI} RPC_FORMAT;
typedef enum {UNKNOWN_COMMAND=0, REGISTER, DEREGISTER, GETLIST} COMMAND_TYPE;

struct SServerInfo
{
	char m_szIPAddress[16];
	RPC_FORMAT m_rpcFormat;
	int m_nPort;
	uint32_t m_uiProgram;;
	uint32_t m_uiVersion;
};

//List of Servers IP address and port
list<SServerInfo> g_serverList;

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

list<SServerInfo>::iterator findServer(string strIPAddress, int nPort)
{
	list<SServerInfo>::iterator it;

	for(it=g_serverList.begin();it!=g_serverList.end();it++)
    {
		if(strIPAddress.compare(it->m_szIPAddress) == 0 && nPort == it->m_nPort)
		{
			return it;
		}
	}

	return g_serverList.end();
}

void addServer(string strIPAddress, int nPort, RPC_FORMAT rpc_format, uint32_t uiProgram, uint32_t uiVersion)
{
	//new ip and port then add 
	if(findServer(strIPAddress, nPort) == g_serverList.end())
	{
		SServerInfo info;
		strcpy(info.m_szIPAddress, strIPAddress.c_str());
		info.m_nPort = nPort;
		info.m_rpcFormat = rpc_format;
		info.m_uiProgram = uiProgram;
		info.m_uiVersion = uiVersion;

		g_serverList.push_back(info);
	}
}

void removeServer(string strIPAddress, int nPort)
{
	list<SServerInfo>::iterator it;

	it = findServer(strIPAddress, nPort);
	
	if(it != g_serverList.end())
	{
		g_serverList.erase(it);
	}
}

string getServerList(RPC_FORMAT rpc_format)
{
	list<SServerInfo>::iterator it;
	string strServerList;
	char szInfo[32];

	for(it=g_serverList.begin();it!=g_serverList.end();it++)
    {
		if(it->m_rpcFormat == rpc_format)
		{
			if(rpc_format == RPC)
			{
				sprintf(szInfo, "%s;%d;%d;", it->m_szIPAddress, it->m_uiProgram, it->m_uiVersion);
				strServerList.append(szInfo);
			}
			else
			{
				sprintf(szInfo, "%s;%d;", it->m_szIPAddress, it->m_nPort);
				strServerList.append(szInfo);
			}
		}
	}
	
	if(strServerList.length() > 0)
	{
		//Remove last ;
		strServerList.erase(strServerList.length()-1,1);
	}

	return strServerList;
}

void showServerList()
{
	int nIndex = 0;
	list<SServerInfo>::iterator it;
	string strServerList;
	char szInfo[32];

	//RPC first
	cout << "------------------------------------------" << endl;
	cout << "-- RPC List " << endl;

	for(it=g_serverList.begin();it!=g_serverList.end();it++)
    {
		if(it->m_rpcFormat == RPC)
		{
			cout << "-- " << ++nIndex << ". " << it->m_szIPAddress << ":" << it->m_nPort << " Program : " << it->m_uiProgram << " Ver : " << it->m_uiVersion << endl;
		}
	}

	if(nIndex == 0)
	{
		cout << "-- none" << endl;
	}

	cout << "------------------------------------------" << endl;
	cout << "-- RMI List " << endl;

	nIndex = 0;

	//RMI first
	for(it=g_serverList.begin();it!=g_serverList.end();it++)
    {
		if(it->m_rpcFormat == RMI)
		{
			cout << "-- " << ++nIndex << ". " << it->m_szIPAddress << ":" << it->m_nPort << " Port : " << it->m_nPort << endl;
		}
	}

	if(nIndex == 0)
	{
		cout << "-- none" << endl;
	}

	cout << "------------------------------------------" << endl;
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

void * heartBeat(void * pData)
{
	int sockfd = 0;
	char szReceivedData[32];
    struct sockaddr_in serv_addr;
	const string strHeartBeat = "heartbeat";
	string strIPAddress;
	int nPort = 0;

    int recvLen, servLen;
    struct hostent *pHostent = NULL;
 
	//Send heartBeat to all other servers.
	list<SServerInfo>::iterator it;
	socklen_t nClientAddr = 0;

	//wait list 2 sec.
	timeval tv;
	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	while(1)
	{
		for(it=g_serverList.begin();it!=g_serverList.end();it++)
		{
		    // sock create
			sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		    if(sockfd == -1)
		    {
		        perror("sock create error. Can't send heartbeat msg to other server.");
				return NULL;
			}
	
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(it->m_nPort);
		    pHostent = gethostbyname(it->m_szIPAddress);

			memcpy((char *)&serv_addr.sin_addr.s_addr, pHostent->h_addr_list[0], pHostent->h_length);

			// send
			sendto(sockfd, strHeartBeat.c_str(), strHeartBeat.length(), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

			// Set Timeout for recv call
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));
		
			recvLen = recvfrom(sockfd, szReceivedData, 32, 0, (struct sockaddr *)&serv_addr, &nClientAddr);
			szReceivedData[recvLen] = '\0';	

//			cout << "Heart bear message : " << szReceivedData << endl;

			//Failed to get heartbeat then will be removed
//			if(strHeartBeat.compare(szReceivedData) != 0)
			if(recvLen <= 0)
			{
				strIPAddress = it->m_szIPAddress;
				nPort = it->m_nPort;
				it--;
				
				cout << strIPAddress << ":" << nPort << "does not reponse for ping. It will be removed." << endl;
				removeServer(strIPAddress, nPort);
			}

			memset(szReceivedData, '\0', 32);
			
			close(sockfd);
		}

		//Wait 5sec
		sleep(5);
	}
}

//Receive register, getlist
void * listen(void * pData)
{
	int listenfd = 0;
	int nState = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[129];
	struct sockaddr_in serv_addr, client_addr;

	string strCommand;
	string strRPCFormat;
	string strIPAddress;

	COMMAND_TYPE command_type = UNKNOWN_COMMAND;
	RPC_FORMAT rpc_format = UNKNOWN_FORMAT;
	int nPort = 0;
	
	//RPC
	uint32_t uiProgram = 0;
	uint32_t uiVersion = 0;

	string strReturn;

	nClientAddr = sizeof(client_addr);

    // socket create
	listenfd = socket(AF_INET, SOCK_DGRAM, 0);

	if(listenfd < 0)
	{
		perror("socket error : ");
        return NULL;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	nState = bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	
	if (nState == -1)
	{
		perror("bind error : ");
		return NULL;
	}

	 //Parsing Address port Info
	CStringTokenizer stringTokenizer;
	
	//Server wiill not finish until all projects are done. 
	while(1)
	{
		nReceivedBytes = recvfrom(listenfd, szReceivedData, 128, 0, (struct sockaddr *)&client_addr, &nClientAddr);

		if(nReceivedBytes == -1)
		{
			perror("recvFrom failed");
			break;
		}

		szReceivedData[nReceivedBytes] = '\0';

		//cout << "Received Message : " << szReceivedData << endl;
	
		//Tokenizer
	    stringTokenizer.Split(szReceivedData, ";");

		//Check Command Type;
		strCommand = stringTokenizer.GetNext();

		if(strcasecmp(strCommand.c_str(), "Register") == 0)
		{
			command_type = REGISTER;
		}
		else if(strcasecmp(strCommand.c_str(), "DeRegister") == 0)
		{
			command_type = DEREGISTER;
		}
		else if(strcasecmp(strCommand.c_str(), "GetList") == 0)
		{
			command_type = GETLIST;
		}
		else
		{
			command_type = UNKNOWN_COMMAND;
			cout << "Unknown command" << endl;
		}

		//Try to check RPC Format 
		strRPCFormat = stringTokenizer.GetNext();

		if(strRPCFormat.compare("RMI") == 0)
		{	
			rpc_format = RMI;
		}
		else if(strRPCFormat.compare("RPC") == 0)
		{
			rpc_format = RPC;
		}
		else
		{
			rpc_format = UNKNOWN_FORMAT;
		}

		if(rpc_format != UNKNOWN_FORMAT)
		{
			strIPAddress = stringTokenizer.GetNext();
			nPort = atoi(stringTokenizer.GetNext().c_str());

			if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
			{
				//Handle Register Command
				if(command_type == REGISTER)
				{
					uiProgram = 0;
					uiVersion = 0;

					if(rpc_format == RPC)
					{
						//RPC
						uiProgram = atol(stringTokenizer.GetNext().c_str());
						uiVersion = atol(stringTokenizer.GetNext().c_str());	
					}

					addServer(strIPAddress.c_str(), nPort, rpc_format, uiProgram, uiVersion);

					if (rpc_format == RPC)
					{
						cout << currentDateTime() << " : Registed RPC " << strIPAddress << ":" << nPort << " ProgID : " << uiProgram << " Vers : " << uiVersion << endl;
					}
					else
					{
						cout << currentDateTime() << " : Registed RMI " << strIPAddress << ":" << nPort << endl;
					}

//					strReturn = "Register success";
				}
				else if(command_type == DEREGISTER)
				{
					removeServer(strIPAddress.c_str(), nPort);

					if (rpc_format == RPC)
					{
						cout << currentDateTime() << " : DeRegisted RPC " << strIPAddress << ":" << nPort << endl;
					}
					else
					{
						cout << currentDateTime() << " : DeRegisted RMI " << strIPAddress << ":" << nPort << endl;
					}
				}
				else if(command_type == GETLIST) //Handle GetList Command
				{
					/*if(findServer(strIPAddress, nPort) == g_serverList.end())
					{
						strReturn = "Your server did not register to registry-server.";
					}
					else*/
					{
						strReturn = getServerList(rpc_format);
		
						if(strReturn.length() > 0)
						{				
		//						cout << "sent list " << strReturn << endl;
						}

						if (rpc_format == RPC)
						{
							cout << currentDateTime() << " : GetList RPC " << strIPAddress << ":" << nPort << endl;
						}
						else
						{
							cout << currentDateTime() << " : GetList RMI " << strIPAddress << ":" << nPort << endl;
						}
					}

					sendto(listenfd, strReturn.c_str(), strReturn.length(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
				}
				else
				{
					cout << currentDateTime() << " : Unknown Command" << endl;
//					strReturn = "Unknown Command";
				}
			}
			else
			{
				cout << currentDateTime() << " : Invaild IP or Port number" << endl;
//				strReturn = "Invaild IP or Port number";
			}
		}	
		else
		{
			cout << currentDateTime() << " : Unknown RPC Format!!!z" << endl;
//			strReturn = "Unknown RPC Format";
		}

//		sendto(listenfd, strReturn.c_str(), strReturn.length(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
   }

   g_serverList.clear();
   close(listenfd);
}

int main(int argc, char *argv[])
{
	//Start super-server
	char szUserInput[512];
	pthread_t listenThread;
	pthread_t heartBeatThread;
	
	//Listener starts. 
	pthread_create(&listenThread, NULL, listen, NULL);
	pthread_create(&heartBeatThread, NULL, heartBeat, NULL);

	cout << "Super-server starts." << endl;

	while(1)
	{
		cin.getline(szUserInput, 512);

		if(strcmp(szUserInput, "list") == 0)
		{
			showServerList();
		}
		else if(strcmp(szUserInput, "exit") == 0)
		{
			break;
		}
		else
		{
			system(szUserInput);
		}
	}

	int nRes = pthread_join(listenThread, NULL);
	nRes = pthread_join(heartBeatThread, NULL);

	return 0;
}
