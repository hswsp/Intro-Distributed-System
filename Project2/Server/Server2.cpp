#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 
#include <stdlib.h> 
#include <sstream>
#include <iostream>

#include "stringTokenizer.h"
#include "Server.h"

#define BUFLEN 8000	//Max length of buffer
using namespace std;
char *serverIP;
typedef enum {UNKNOWN_COMMAND_QUM=10, FORWARD_QUM,RECEIVEVER, REQUESTVER, OPERATION_READ,OPERATION_WRITE,OPERATION_REPLY, OPERATION_SYNCH} COMMAND_TYPE_QUM;
int seq_server::ID = 0;
queue<string> seq_server::requestQ;

qum_server:: qum_server(bool Coord):Server(Coord)
{
    cout << "Input N, Nr, Nw, Spliteed by ;";
    char buffer[BUFLEN];
    cin.getline(buffer, BUFLEN);

    CStringTokenizer bufferTokenizer;
    bufferTokenizer.Split(buffer, ";");
    
    this->N = stoi(bufferTokenizer.GetNext());
    this->Nr = stoi(bufferTokenizer.GetNext());
    this->Nw = stoi(bufferTokenizer.GetNext());


}

vector<SServerInfo> qum_server::selectRandomServer(int size, int select, vector<SServerInfo> IPs){
    
    int quorum[size];
    for(int i = 0; i < size; i++){
        quorum[i] = i;
    }
    for(int i = size-1; i > 0;--i){
        swap(quorum[i], quorum[rand()%i]);
    }

    // select num of index
    vector<SServerInfo> serverSelected;
    for(int i = 0;i<select;i++){
        // select the quorum[i] index of server
        vector<SServerInfo>::iterator current = findServerByIndex(quorum[i], IPs);
        SServerInfo newServerInfo;
        newServerInfo.m_nPort = current->m_nPort;
        memcpy(newServerInfo.m_szIPAddress, current->m_szIPAddress, sizeof(newServerInfo.m_szIPAddress));
        serverSelected.push_back(newServerInfo);
        
    }
    return serverSelected;
}

vector<SServerInfo>::iterator qum_server::findServerByIndex(int index, vector<SServerInfo> IPs)
{
	vector<SServerInfo>::iterator it = IPs.begin();
    advance(it,index);
    return it;
}

bool qum_server::isValidQR(int N, int Nr, int Nw){
    if(Nr + Nw > N){
        return true;
    } else {
        cout << "Invalid Qr" << endl;
        return false;
    }
}

bool qum_server::isValidQW(int N, int Nr, int Nw){
    if(Nw > N / 2){
        return true;
    } else {
        cout << "Invalid Qw" << endl;
        return false;
    }
}

bool qum_server::isValidQ(int N, int Nr, int Nw){
    if(isValidQR(N, Nr, Nw) && isValidQW(N, Nr, Nw)){
        return true;
    } else {
        return false;
    }
}

vector<SServerInfo>::iterator qum_server::getNewestServer(vector<SServerInfo> serverSelected){
    int selectedServerSize = serverSelected.size();
    vector<SServerInfo>::iterator it = serverSelected.begin();

    vector<SServerInfo>::iterator newest;
    int maxVer = 0;

    string ip = checkip();
    string port = to_string(REQUESTPORT);
    string command = "REQUESTVER";

    command.append(ip);
    command.append(port);

    
    for(int i = 0; i < selectedServerSize;i++){
        UDP_send(it->m_szIPAddress,PRIMARYPORT,command.c_str());
        cout << "Send Request for version to " << it->m_szIPAddress << endl;
        string ReceivedData;
        ReceivedData = receive(REQUESTPORT);
        cout << "--->Received ver num: " << ReceivedData << endl;
        int currentVer = atoi(ReceivedData.c_str());
        if(currentVer > maxVer){
            newest = it;
        }
    }
    cout << "Newest server is: " << newest->m_szIPAddress << endl;
    return newest;

}



string qum_server::generatePrefix(const SServerInfo client_addr, string request, string TYPE){
    TYPE.append(";");
    TYPE.append(client_addr.m_szIPAddress);
    TYPE.append(";");
    TYPE.append(to_string(client_addr.m_nPort));
    TYPE.append(";");
    TYPE.append(request);
    return TYPE;
}




void qum_server::Post_handler(const SServerInfo&  client_addr, string request){
    // [POST;host;port;content]
    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, request, "POST");
    requestCont.append(requestLast);
    cout << "Request Content: " << requestCont << endl;

    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}

void qum_server::Read_handler(const SServerInfo&  client_addr){
     // [POST;host;port;content]
    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, "", "READ");
    requestCont.append(requestLast);
    cout << "Request Content: " << requestCont << endl;
   
    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}
void qum_server::Choose_handler(const SServerInfo&  client_addr, string request){

} 
void qum_server::Reply_handler(const SServerInfo&  client_addr, string request){
    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, "", "REPLY");
    requestCont.append(requestLast);
    cout << "Request Content: " << requestCont << endl;
   
    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}

void qum_server::FORWARD_handler(string request){
    // situation: primary receive forward from a server
    // request: [POST/READ/REPLY; clienthost;clientport]
    CStringTokenizer requestTokenizer;
    requestTokenizer.Split(request, ";");
    string command = requestTokenizer.GetNext();

    if (strcasecmp(command.c_str(), "READ") == 0){
        // read quorum
        vector<SServerInfo> serverList = selectRandomServer(this->N, this->Nr, SIPs);
        vector<SServerInfo>::iterator newest = getNewestServer(serverList);

        string clienthost = requestTokenizer.GetNext();
        string clientport = requestTokenizer.GetNext();


        // get article content
        string command = "OPERATION_READ";
        command.append(clienthost);
        command.append(clientport);
        cout << "READ: Ask Server " << newest->m_szIPAddress << "Send articles to client" << endl;
        cout << "   Command: " << command << endl;
        UDP_send(newest->m_szIPAddress, PRIMARYPORT, command.c_str());
    } else if (strcasecmp(command.c_str(), "POST") == 0){

    } else if (strcasecmp(command.c_str(), "REPLY") == 0){

    } else {
        cout << "Error: unknown command at primary" << endl;
    }
}




void * qum_server::quorum_listenImpl(void * pData){
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    string localip = checkip();
    string strCommand;
    string strIPAddress;
    int nPort = 0;
    COMMAND_TYPE_QUM command_type = UNKNOWN_COMMAND_QUM;

	nClientAddr = sizeof(client_addr);
    // socket create
	listenfd = UDP_bind(PRIMARYPORT);

	 //Parsing Address port Info
	CStringTokenizer stringTokenizer;
	//keep listening for data
	//wait list 10 sec.
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;

    while(1){
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

        // Tokenizer the message
        
        stringTokenizer.Split(szReceivedData, ";");
        strCommand = stringTokenizer.GetNext();

        
        if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "FORWARD_QUM") == 0) // only for primary
		{
			command_type = FORWARD_QUM;
		}
		else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "RECEIVEVER") == 0) // only for primary
		{
			command_type = RECEIVEVER;
		}
        else if(strcasecmp(strCommand.c_str(), "REQUESTVER") == 0) 
		{
			command_type = REQUESTVER;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_READ") == 0)
		{
			command_type = OPERATION_READ;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_WRITE") == 0)
		{
			command_type = OPERATION_WRITE;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_REPLY") == 0)
		{
			command_type = OPERATION_REPLY;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_SYNCH") == 0)
		{
			command_type = OPERATION_SYNCH;
		}

        // unknown command
        else
		{
			command_type = UNKNOWN_COMMAND_QUM;
			cout << "Unknown command" << endl;
		}

        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());

        if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true){
            /*
            cout<< "ReceivedData in listen: "<<ReceivedData<<endl;
            for(int i = 0;i<3;++i)
            {
               ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1); 
            }
            */
            string request = ReceivedData;

            // REceivedData -> [host;port;content]

            if(command_type == FORWARD_QUM){ // only primary

                // deal with received forward message
                FORWARD_handler(request);

            } else if (command_type == RECEIVEVER){ // only primary
                // ditto
            } else if (command_type == REQUESTVER){
                // get version number and send back
                UDP_send(strIPAddress.c_str(), nPort, (to_string(this->VER).c_str()));
            } else if (command_type == OPERATION_READ){
                // send article to the target server with designated port and host
                cout << "OP_READ: Send article to client" << endl;
                this->sendArticle(strIPAddress.c_str(), nPort);
            }
        } else {
            cout << currentDateTime() << " : Invaild IP or Port number" << endl;
        }
    }
    close(listenfd);
    return nullptr;
}

void*  qum_server::quorum_listen(void * pData)
{
    mypara *pstru;
    qum_server* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<qum_server *>(pstru -> _this))
    {
        
        pThis->quorum_listenImpl((void *)(pstru->Is_live));
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}


