#include <stdlib.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sstream>
#include <typeinfo>
#include <unistd.h>
#include <fstream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <bits/stdc++.h> 
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>

#include "server.h"
#include "xFs_system.h"
#include "stringTokenizer.h"

using namespace std;



FileInfo::FileInfo(string name, string ck):checksum(ck)
{
    int i;
    for(i=0;i<name.length();i++)this->filename[i] = name[i];
    filename[i] = '\0';
}
void* Server::RecvList(void * pData)
{
    mypara *pstru;
    Server* pThis;
    pstru = (struct mypara *) pData;
    pThis = static_cast<Server *>(pstru -> _this);
    pThis->RecvListImp((void *)(pstru->Is_live), UPDATEPORT);
    
    return NULL;
}
void* Server::Heartbeat(void * pData)
{
    mypara *pstru;
    Server* pThis;
    pstru = (struct mypara *) pData;
    pThis = static_cast<Server *>(pstru -> _this);
    pThis->HeartbeatImp((void *)(pstru->Is_live), PORT_SERVER_HEARTBEAT);
    
    return NULL;
}


void *Server::FindRes_Getload(void * pData)
{
    mypara *pstru;
    Server* pThis;
    pstru = (struct mypara *) pData;
    pThis = static_cast<Server *>(pstru -> _this);
    pThis->FindResImp((void *)(pstru->Is_live), FINDPORT);
    return NULL;
}

struct clnMatch  // function object for compare, not used yet
{
    string m_id;
    clnMatch(string id):m_id(id){};

    bool operator()(const ClientInfo& si)  
    {
        return si.fileMatch(m_id);
    }
};

struct filecmp
{
    string filename;
    filecmp(const string& id):filename(id){};
    bool operator()(const FileInfo& si)
    {
		return (strcmp(filename.c_str(),si.filename)==0);
	}
};

bool ClientInfo::fileMatch(const string& fname) const
{
    vector<FileInfo>::const_iterator it = find_if(this->fileList.begin(),this->fileList.end(),filecmp(fname));
	return (it!=this->fileList.end());
}


void Server::FindResImp(void * pData,int port)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    string strIPAddress;
    int nPort = 0;
    string strCommand;
	listenfd = UDP_bind(port);

	//wait list 10 sec.
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
    
    //Parsing Address port Info
	CStringTokenizer stringTokenizer;
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
        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());
        if("FIND"==strCommand)
        {
            string filename = stringTokenizer.GetNext();
            string retrninfo = "";
            list<ClientInfo>::iterator it;
            for(it = this->clientInfoList.begin();it!= this->clientInfoList.end();++it)
            {
                if(it->fileMatch(filename))
                {
                    retrninfo.append(to_string(it->id)).append(";");
                    retrninfo.append(it->host).append(";");
                }
            }
            if(!retrninfo.empty())
            {
                retrninfo.pop_back();//delete last ";"
            } else {
                retrninfo.append("EMPTY");
            }
            UDP_send(strIPAddress.c_str(), nPort, retrninfo.c_str());
        }

    }


    pthread_exit(NULL);
}
void Server::RecvListImp(void * pData,int port)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    string clnt_IPAddress;
    int clnt_Port = 0;
    string strCommand;
    int clntID = -1;
	listenfd = UDP_bind(port);

	//wait list 10 sec.
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
    
    //Parsing Address port Info
	CStringTokenizer stringTokenizer;
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
        //cout << "---> Received Data: " << ReceivedData << endl;
        //Tokenizer
	    stringTokenizer.Split(szReceivedData, ";");

		//Check Command Type;
		strCommand = stringTokenizer.GetNext();
        clnt_IPAddress = stringTokenizer.GetNext();
        clnt_Port = atoi(stringTokenizer.GetNext().c_str());
        clntID = atoi(stringTokenizer.GetNext().c_str());
        if("UPDATELIST"==strCommand)
        {
            cout<<" -----------------Receive updatelist from "<<clnt_IPAddress<<":"<<clnt_Port<<"--------------"<<endl;
            ClientInfo clnt(clntID,clnt_IPAddress,clnt_Port);
            string filename,md5;
            list<ClientInfo>::iterator it = find_if(this->clientInfoList.begin(),this->clientInfoList.end(),ClientInfo(clnt));
            if(it==this->clientInfoList.end())//Not find, add one
            {
                cout << "Add new Node --> ID: " << clnt.id << "   Addr: " << clnt.host << "   Port: " << clnt.port << endl;
                this->clientInfoList.push_back(clnt);
                it = clientInfoList.end();
                --it;
            }
            while((filename = stringTokenizer.GetNext())!="")
            {
                md5 = stringTokenizer.GetNext();
                it->fileList.push_back(FileInfo(filename,md5));
                cout<<"file: "<<filename<<" "<<"MD5: "<<md5<<endl;
            }
            
            UDP_send(clnt_IPAddress.c_str(), clnt_Port, "SUCCESS");
        }
    }

    pthread_exit(NULL);
}
void Server::HeartbeatImp(void * pData,int port)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    string clnt_IPAddress;
    int clnt_Port = 0;
    string strCommand;
    
	listenfd = UDP_bind(port);

	//wait list 10 sec.
	timeval tv;
	tv.tv_sec  = 100;
	tv.tv_usec = 0;
    
    //Parsing Address port Info
	CStringTokenizer stringTokenizer;
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
        clnt_IPAddress = stringTokenizer.GetNext();
        clnt_Port = atoi(stringTokenizer.GetNext().c_str());

        if("HEARTBEAT"==strCommand)
        {
            //cout<<" -----------------Receive HeartBeat from "<<clnt_IPAddress<<":"<<clnt_Port<<"--------------"<<endl;
            ClientInfo clnt;
            clnt.host = clnt_IPAddress;
            clnt.port = clnt_Port;
            UDP_send(clnt_IPAddress.c_str(), clnt_Port, "SUCCESS");
        }
    }

    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {

    cout << "Start Server" << endl;

    srand( (unsigned)time( NULL ) );
    bool Is_live= true;
    struct mypara pstru;
    Server* node;


    // thread
    pthread_t listen_find_thread;
    pthread_t listen_Update_thread;
    pthread_t heartbeat_thread;
    
    node = new Server();

    if (argc < 1) {
        fprintf(stderr, "usage: %s tracking server message \n", argv[0]);
        exit(-1);
    }
    string localip = checkip();
    cout<< "The IP address of this server is: "<<localip<<endl;
    
    cout<< "----------------Server running------------------ "<<endl;
    pstru._this = node;
    pstru.Is_live = &Is_live;
    
    pthread_create(&listen_find_thread, NULL, Server::FindRes_Getload, (void*)&pstru);
    pthread_create(&listen_Update_thread, NULL, Server::RecvList, (void*)&pstru);
    pthread_create(&heartbeat_thread, NULL, Server::Heartbeat, (void*)&pstru);
    char buffer[BUFLEN];
    while(1)
    {
        printf("------\nCommand:\nexit: to shut down the tracking server \nlist: to show all connected clients \n");/// synch
        std::cin.getline(buffer, BUFLEN);
		if(strcmp(buffer,"exit") == 0){
            cout<<"-------------exit server--------------------------"<<endl;
            Is_live = false;
            break;
        }
        if(strcmp(buffer,"list") == 0){
            cout<<"-------------clients--------------------------"<<endl;
            for(ClientInfo clnt:node->clientInfoList)
            {
                cout<<"client "<<clnt.id<<" "<<clnt.host<<":"<<clnt.port<<endl;
            }
        }
        else {
			system(buffer);
		}
    }
    int nRes1 = pthread_join(listen_find_thread, NULL);
    int nRes2 = pthread_join(listen_Update_thread, NULL);
    int nRes3 = pthread_join(heartbeat_thread, NULL);
    cout<< "----------------Server exit------------------ "<<endl;
    return 0;


}