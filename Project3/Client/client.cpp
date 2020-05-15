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
#include <time.h> 
#include <algorithm>
#include <openssl/md5.h>
#include <dirent.h>
#include "client.h"
#include "xFs_system.h"
#include "stringTokenizer.h"

using namespace std;

typedef enum {UNKNOWN_COMMAND = 0, GETLOAD, DOWNLOAD, DEBUG} COMMAND_TYPE;
const int randM = 5;
Client::Client(){
    // clear buffer

}

 communicate_data communicate_data::operator =(const communicate_data& b)
 {
    if(this!= &b)
    {
        
        this->is_detach = b.is_detach;
        this->request = b.request; 
        this->addr_back = b.addr_back; 
        this->port_back = b.port_back; 
        this->result = b.result;  
        this->_this = b._this; //has risk
       
    }
     
    return *this;
    
 }
communicate_data::communicate_data(const communicate_data& b)
{
    this->is_detach = b.is_detach;
    this->request = b.request;
    this->addr_back = b.addr_back;
    this->port_back = b.port_back;
    this->result = b.result;
    this->_this = b._this; // has risk
}


// Start Up
void Client::StartUp(string addr, int id, string sharedDirectory, string latencyFile){
    SetServerInfo(addr, id);
    this->nodeID = id;

    for(int i = DOWNLOAD_POOL_START ; i <= DOWNLOAD_POOL_END ; i++){
        this->downloadPortPool.push_back(i);
    }

    LoadFileList(sharedDirectory);
    LoadLatencyList(latencyFile);
    UpdateList();

}

string Client::Communicate(string operationName, string serverAddr, int serverPort, int clientPort, string request){
    //cout << "---> Start " << operationName << "...." << endl;
    string localip = checkip();
    char command[BUFLEN];
    snprintf(command,sizeof(command),"%s;%s;%d;%s",operationName.c_str(),localip.c_str(), clientPort, const_cast<char*>(request.c_str()));
    UDP_send(serverAddr.c_str(), serverPort, command);
    string ReceivedData;
    ReceivedData = receive(clientPort);
    if(ReceivedData == "FAIL"){
        cout << " ---> Receive Data Failed <---" << endl;
        return "";
    }

    //cout << "---> End " << operationName << "...." << endl;

    return ReceivedData;
}

void Client::Send(string operationName, string serverAddr, int serverPort, int clientPort, string request){
    //cout << "---> Start " << operationName << "...." << endl;
    string localip = checkip();
    char command[BUFLEN];
    snprintf(command,sizeof(command),"%s;%s;%d;%s",operationName.c_str(),localip.c_str(), clientPort, const_cast<char*>(request.c_str()));
    UDP_send(serverAddr.c_str(), serverPort, command);
}


// Find

string Client::Find(string filename){
    
    // sleep for latency

    string receivedData =  Communicate("FIND", trackingServerInfo.addr, PORT_SERVER_FIND,PORT_CLIENT_FIND_RETURN,filename);
    if(receivedData == "EMPTY"){
        cout << "Can't find file " << filename << " globally" << endl;
    }
    return receivedData;
   
}

void Client::LoadServerInfo(string receivedData, list<ServerInfo> &nodeInfoList){
    // deal with received data
    CStringTokenizer nodeInfoTokenizer;
    nodeInfoTokenizer.Split(receivedData, ";");
    string nextID, nextAddr;

    // clear buffer before use

    while((nextID = nodeInfoTokenizer.GetNext()) != ""){
        //cout << "node id: " << nextID << endl;
        
        int nextID_int = stoi(nextID);
        nextAddr = nodeInfoTokenizer.GetNext();
        //cout << "node addr: " << nextAddr << endl;

        ServerInfo newNodeInfo;
        newNodeInfo.addr = nextAddr;
        newNodeInfo.id = nextID_int;

        nodeInfoList.push_back(newNodeInfo); 
        
    }
}

// Download


void Client::Download(string filename){

    string receivedData; 
    communicate_data c_data;

    receivedData = Find(filename);
    if(receivedData == ""){
        cout << "Error: Tracking Server is not online "<< endl;
        return;
    } else if(receivedData == "EMPTY"){
        cout << "Exit Download" << endl;
        return;
    }


    list<ServerInfo> nodeInfoList;
    LoadServerInfo(receivedData, nodeInfoList);

    int size = nodeInfoList.size();
    cout << "Total Available num: " << size << endl;
    cout << "Available node for file " << filename << ": " << endl;
    this->localLatencyGraph.shortestPath(this->nodeID, this->dist);

    while(nodeInfoList.size() != 0){

        // overview
        int minload_id = GetMinLoadNode(nodeInfoList);
        string nodeAddr = GetNodeAddr(minload_id, nodeInfoList);
        cout << "Connect node " << minload_id << " with addr " << nodeAddr << "...." << endl;
        bool result;

        // fill forward list
        vector<ServerInfo> forwardList;
        if(minload_id == this->nodeID)
        {
            cout << "File already exist at local store" << endl;
            RemoveNodeInfo(minload_id, nodeInfoList);
            continue;  // ONLY FOR TEST, CHANGE TO BREAK AT LAST 
        } else 
        {
            FillForwardList(forwardList, nodeInfoList, minload_id);
        }

        string request = PackForwardRequest(forwardList, filename);
       
        // CORE: ripple download request

        pthread_t request_download_thread;
        
        c_data.addr_back = "host";
        c_data.port_back = 0;
        c_data.is_detach = false;
        c_data._this = this;
        c_data.request = request;
        // manipulate load here. Load will be minused within thread.
        pthread_create(&request_download_thread,NULL, Client_communicate_Download, (void *) &c_data);

        pthread_join(request_download_thread, NULL);
  
        receivedData = c_data.result;

        cout << receivedData <<endl;
        // Validate and fill result
        if(receivedData != ""){
            // if success
            result = ValidatePackedData(receivedData, filename);
            if(!result) 
            {
                cerr<<"checksum not match!"<<endl;// not match
                continue; 
            }

        } else 
        {

            // directly Connect (Not new thread?)
            string request_direct = nodeAddr + ";$;" + filename;
            cout << "Shortest path failed, start Direct connect request: " << request_direct << endl;

            string receivedData_direct = ForwardDownload(request_direct);
            if(receivedData_direct != "")
            {
               result = ValidatePackedData(receivedData_direct, filename);
               if(!result) 
               {
                   cerr<<"checksum not match!"<<endl;// not match
                   continue; 
               }
            } 
            else 
            {
                // change another node
               // cout << "Minload_id Node " << minload_id << "failed. (addr: )" << nodeAddr << endl;
                cout << "Switch to next available minNode..." << endl;
                result = false;
            }
        }

        if(result)
        {
            break;
        } else 
        {
            RemoveNodeInfo(minload_id, nodeInfoList);
        }
    }

    if(nodeInfoList.size() == 0){
        // The resource is globally unavailable.
        cout << "The file "<< filename << "is globally unavailable." << endl;
    }


}


bool Client::ValidatePackedData(string receivedData, string filename){
     //cout << "   ----> Received: " << receivedData << endl;
    FileInfo thisFileInfo = UnpackFileInfo(filename, receivedData); // now receivedData will be file content
    //cout << "       -> new Filename: " << thisFileInfo.filename << endl;
    //cout << "       -> checksum: " << thisFileInfo.checksum << endl;
    if( CompareMD5(thisFileInfo.checksum, receivedData)){
        cout << "checksum success, ready for write file and update list" << endl;
        cout << "Complete Filename: " << (this->sharedDirectory + filename) << endl;
        String2File((this->sharedDirectory + filename), receivedData);
        UpdateList();  
        cout << "Complete Write File " << thisFileInfo.filename << " locally" << endl;
        return true;
    } else {
        cout << "Fail to write File " << thisFileInfo.filename << ": checksum incorrect" << endl;
        return false;
    }
    
}



int Client::GetPort(){

    // lock and fetch port (prevent multiple thread access port pool at the same time)
    pthread_mutex_lock(&this->client_mutex);

    // new pthread?
    this->Load += 1;
    int listen_port = downloadPortPool.front();
    downloadPortPool.pop_front();
    //cout << "---> (LOAD + 1) Fetch listen port: " << listen_port << endl;

    pthread_mutex_unlock(&this->client_mutex);

    return listen_port;

}

void Client::ReturnPort(int listen_port){

    // return port after use
    pthread_mutex_lock(&this->client_mutex);

    downloadPortPool.push_back(listen_port);
    //downloadPortPool.push_front(listen_port);
    this->Load -= 1;
    //cout << "<--- (LOAD - 1) Return listen port: " << listen_port << endl;

    pthread_mutex_unlock(&this->client_mutex);
}



string Client::GetNextForwardAddr(string &request){
    CStringTokenizer forwardTokenizer;
    forwardTokenizer.Split(request, ";");
    string addr = forwardTokenizer.GetNext();

    request = request.substr(request.find_first_of(";") + 1);
    //cout << "   ----> Remain Request: " << request << endl;
    return addr;
}



string Client::ForwardDownload(string &request){
    // ForwardRequest: [addr1;addr2;addr3;...;$;content]

    // send addr1 with the rest content (stack)
    
    string addr = GetNextForwardAddr(request);
    if(addr != "$"){
        int listen_port = GetPort();

        if(!isValidPort(listen_port))
        {
            cout << "Invalid port!" << endl;
            
        } else 
        {
            //cout << "Current port is valid for use" << endl;
        }

        string receivedData = Communicate("DOWNLOAD", addr.c_str(), PORT_CLIENT_DOWNLOAD,listen_port,request); //block
        //artificially test
        if(this->nodeID == 1)
        {
            int x = rand()%randM;
            if(x==1) // add noise
            {
                receivedData.replace(receivedData.length()-2, 1,"k");
            }
        }


        if(receivedData == ""){ // send fault feedback
            ReturnPort(listen_port);
            return "";
        } else {
            ReturnPort(listen_port); // send packedfile feedback
            return receivedData;
        }

    } else { // find file locally
        string filepack = PackFileInfo(this->sharedDirectory + request);
        return filepack; // return "" if cant find file
    }

}



void Client::FillForwardList(vector<ServerInfo> &forwardList, list<ServerInfo> nodeInfoList, int minload_id){

    // fill route in nodeInfoList
    int src = nodeID;
    int end = minload_id;
    

    //print
   // cout << "--- Shortest Path Summary ---" << endl;

    if(this->dist[end] > MAX_LATENCY){
        cout << "Unreachable within max latency to Node " << end << endl;
        return;
    }

    //cout<<" End: " <<  end << " Distance: " <<this->dist[end]<<endl;
   // cout<<" The path is:"<<endl;
   // cout<<src;

    for(int j=0;this->localLatencyGraph.path[src][end][j]!=-1;j++)
    {
        int id = this->localLatencyGraph.path[src][end][j];
       // cout<< "->"<< id;
        ServerInfo newNode;
        newNode.id = id;
        forwardList.push_back(newNode);
    }
    //cout<<endl;
        

   // cout << "Shortest path to Node " << end << " is: " << this->dist[end] << endl;



    LoadInfoListByID(forwardList, nodeInfoList);
    cout << "The decided route is: " << endl;
    cout << nodeID << " (" << checkip() << ") ";
    for(vector<ServerInfo>::iterator it = forwardList.begin(); it != forwardList.end() ; it++){
        cout << "->"<< it->id << " (" << it->addr << ") ";
    }
    cout << endl;

}



string Client::PackForwardRequest(vector<ServerInfo> forwardList, string filename){
    
    string request = "";

    for(vector<ServerInfo>::iterator it = forwardList.begin(); it != forwardList.end();it++){
        request.append(it->addr);
        request.append(";");
    }

    request.append("$;");
    request.append(filename);

    return request;
}





void Client::RemoveNodeInfo(int nodeID, list<ServerInfo> &nodeInfoList){
    for(list<ServerInfo>::iterator it = nodeInfoList.begin(); it != nodeInfoList.end(); it++){
        if(it->id == nodeID){
            nodeInfoList.erase(it);
            return;
        }
    }

    // no find
    cout << "RemoveNodeInfo: Can not find node " << nodeID << " in current node info list" << endl;
    return ;
}

struct compbyV
{
    bool operator()(const pair<int,int> &a,const pair<int,int> &b)
    {
        return a.second<b.second;
    }
};
bool cmp_p(const pair< pair<int,int>,int>  &a,const pair< pair<int,int>,int>  &b)
{
    return a.second<b.second;
}

int Client::GetMinLoadNode(list<ServerInfo> nodeInfoList){ // remove the minload nodeInfo after found
    int min_load = MAX_LOAD;
    int min_id = -1;
    vector< pair<int,int> > loadV; // (id,load) map<int,int,compbyV>
    list<ServerInfo>::iterator min_it;

    for(list<ServerInfo>::iterator it = nodeInfoList.begin(); it != nodeInfoList.end(); it++){
        cout << "   Node: " << it->id;
        int it_load = GetLoad(it->id, nodeInfoList);
        cout << "   Load: " << it_load << endl;
        loadV.push_back(make_pair(it->id,it_load));
    }
    sort(loadV.begin(),loadV.end(),compbyV());
    pair<int,int> min_pair = loadV.front();
    int high = upper_bound(loadV.begin(),loadV.end(),min_pair,compbyV())-loadV.begin();
    int low = lower_bound(loadV.begin(),loadV.end(),min_pair,compbyV())-loadV.begin();
    if(high == low + 1)//only one, load least fisrt
    {
        min_load = min_pair.second;
        min_id = min_pair.first;
    }
    else
    {
       vector<pair< pair<int,int>,int> > pathV; //((id,load),dist) 
       for(int i = low;i<high;++i)
       {
           pathV.push_back(make_pair(loadV[i],this->dist[loadV[i].first])); 
       }
       sort(pathV.begin(),pathV.end(),cmp_p);
       min_id = pathV.front().first.first;
       min_load = pathV.front().first.second;
    }
    
    cout << "Min load node: " << min_id << " with load: " << min_load << endl;

    return min_id;
}


void Client::LoadGlobalNodeInfo(){

    global_nodeInfoList.clear(); // necessary
    string receivedData = Find("global.txt");
    if(receivedData == ""){
        cout << "Tracking Server is not online "<< endl;
        return ;
    }
    LoadServerInfo(receivedData, this->global_nodeInfoList);
}




int Client::GetLoad(int nodeID, list<ServerInfo> nodeInfoList){
    
    // already get info
    string nodeAddr = GetNodeAddr(nodeID, nodeInfoList);
    if(nodeAddr == ""){
        return MAX_LOAD;
    }
    string receivedData = Communicate("GETLOAD",nodeAddr,PORT_CLIENT_GETLOAD,PORT_CLIENT_GETLOAD_RETURN,""); // request for getload is empty

    try{
        return stoi(receivedData);
    } catch (invalid_argument){
        cout << "Error: Not int for received data" << endl;
        return MAX_LOAD;
    }

}


int Client::GetLoad(int nodeID){

    if(nodeID == this->nodeID){
        return this->Load;
    }
    return GetLoad(nodeID, this->global_nodeInfoList);

}

string Client::GetNodeAddr(int nodeID, list<ServerInfo> nodeInfoList){

    for(list<ServerInfo>::iterator it = nodeInfoList.begin(); it != nodeInfoList.end(); it++){
        if(it->id == nodeID){
            return it->addr;
        }
    }

    // no find
    cout << "GetNodeAddr: Can not find node " << nodeID << " in current node info list" << endl;
    return "";
}


int Client::GetAddrNode(string Addr, list<ServerInfo> nodeInfoList){

    for(list<ServerInfo>::iterator it = nodeInfoList.begin(); it != nodeInfoList.end(); it++){
        if(it->addr == Addr){
            return it->id;
        }
    }

    // no find
    cout << "Can not find Addr" << Addr << "in current node info list" << endl;
    return -1;
}


void Client::LoadInfoListByID(vector<ServerInfo> &forwardList, list<ServerInfo> nodeInfoList){
    for(vector<ServerInfo>::iterator it = forwardList.begin(); it != forwardList.end(); it++){
        it->addr = GetNodeAddr(it->id, nodeInfoList);
        if(it->addr == ""){
            perror("invalid addr by getNodeaddr: \n");
            // CHEATING
            it->addr = checkip().c_str();
        }
    }
}



void Client::UpdateList(){

    string request = PackFilelistInfo();
    string receivedData = Communicate("UPDATELIST",trackingServerInfo.addr,PORT_SERVER_UPDATELIST,PORT_CLIENT_UPLOADLIST_RETURN,request);
    
    // deal with feedback

    if(receivedData == "SUCCESS"){
        cout << "Successfully Updated Local File List" << endl;
        this->is_trackingServer_online = true;
    }
    if(receivedData == ""){
        cout << "Fail to Update Local File List" << endl;
        this->is_trackingServer_online = false;
    }


}


string Client::PackFilelistInfo(){
    cout << "File num: " << localFileList.size() << endl;
    string fileInfo;
    fileInfo.append(to_string(this->nodeID));
    fileInfo.append(";");
    for(list<FileInfo>::iterator it = localFileList.begin(); it != localFileList.end(); it++){
        char request[MD5_SIZE * 2 + MAX_FILENAME_SIZE];
        snprintf(request,sizeof(request), "%s;%s;", it->filename, it->checksum.c_str());
        fileInfo.append(string(request));
    }
    return string(fileInfo);

}


// Listen Part (For getLoad and Download request)

void * Client::Client_listenImpl(void * pData, int listenPort)
{
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

    communicate_data c_data;

    COMMAND_TYPE command_type = UNKNOWN_COMMAND;

	nClientAddr = sizeof(client_addr);


	listenfd = UDP_bind(listenPort);

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

       if(strcasecmp(strCommand.c_str(), "GETLOAD") == 0) // only for primary
		{
			command_type = GETLOAD;
		}
        else if(strcasecmp(strCommand.c_str(), "DOWNLOAD") == 0) 
		{
			command_type = DOWNLOAD;
		}
        else if(strcasecmp(strCommand.c_str(), "DEBUG") == 0) 
		{
			command_type = DEBUG;
		}
		else
		{
			command_type = UNKNOWN_COMMAND;
			cout << "Unknown command" << endl;
		}
        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());

        if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
        {
            for(int i = 0;i<3;++i)
            {
               ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1); 
            }
            string request = ReceivedData;

            if(command_type == GETLOAD) // send articles to new primary
            {
                // just return get load
                //cout << "(command GETLOAD: sent load feedback: " << Load <<  ")" << endl;
                UDP_send(strIPAddress.c_str(), nPort, to_string(this->Load).c_str());

            } else if (command_type == DEBUG){
                cout <<"(command DEBUG: download request (which is filename: "<< request << " ))" << endl;
                Download(request);

                // no return value
                

            }
            else if(command_type == DOWNLOAD)
            {
                // request -> filename
                cout <<"(command DOWNLOAD: get file packed and sent)" << endl;
                cout << "-> From Addr "<< strIPAddress << " and Port" << nPort << endl;
                
                pthread_t forward_download_thread;
                
                c_data.addr_back = strIPAddress;
                c_data.port_back = nPort;
                c_data.is_detach = true;
                c_data._this = this;
                c_data.request = request;
                // add info in list for reference       
                pthread_create(&forward_download_thread,NULL, Client_communicate_Download, (void *) &c_data);
                
            }
            
        }
        else{
            cout << currentDateTime() << " : Invaild IP or Port number" << endl;
        }
   }
  
   close(listenfd);
   return nullptr;
}




void* Client::Client_listen_Download(void * pData)
{
    mypara *pstru;
    Client* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<Client *>(pstru -> _this))
    {
        
        pThis->Client_listenImpl((void *)(pstru->Is_live), PORT_CLIENT_DOWNLOAD);
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}

void* Client::Client_listen_Getload(void * pData)
{
    mypara *pstru;
    Client* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<Client *>(pstru -> _this))
    {
        
        pThis->Client_listenImpl((void *)(pstru->Is_live), PORT_CLIENT_GETLOAD);
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}

void* Client::Client_listen_Debug(void * pData)
{
    mypara *pstru;
    Client* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<Client *>(pstru -> _this))
    {
        
        pThis->Client_listenImpl((void *)(pstru->Is_live), PORT_CLIENT_DEBUG);
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}

// shit , i don't think this will work, the multiple threads cant use the same FIXED client port!
void* Client::Client_download(void * pData)
{
    pthread_detach(pthread_self());
    mypara *pstru;
    Client* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<Client *>(pstru -> _this))
    {
        
        pThis->Download(pstru->filename);
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}


static void StartDownloadThread(mypara pstru, string filename){

    pstru.filename = filename;
    pthread_t downloadThread;
    pthread_create(&downloadThread, NULL, Client::Client_download, (void*)&pstru);

}


void* Client::Client_heartbeat(void * pData){


    mypara *pstru;
    Client* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<Client *>(pstru -> _this)){
        while(1){
            sleep(5);

            if(pstru->_this->is_trackingServer_online == false){
                cout << " ### HEARTBEAT: Reconnecting Server " << pstru->_this->trackingServerInfo.addr << "...."<< endl;
                string receivedData = pstru->_this->Communicate("HEARTBEAT",pstru->_this->trackingServerInfo.addr,PORT_SERVER_HEARTBEAT,PORT_CLIENT_HEARTBEAT_RETURN, "");
                if(receivedData == "SUCCESS"){
                    cout << " ### HEARTBEAT: Tracking Server Back Online - Updatelist to recover info" << endl;
                    pstru->_this->is_trackingServer_online = true;
                    pstru->_this->UpdateList();
                } else {
                    cout << " ### HEARTBEAT: Tracking Server is Down: Reconnect at next ping" << endl;
                    
                }
            }


            if(pstru->_this->is_trackingServer_online == true){
                string receivedData = pstru->_this->Communicate("HEARTBEAT",pstru->_this->trackingServerInfo.addr,PORT_SERVER_HEARTBEAT,PORT_CLIENT_HEARTBEAT_RETURN, "");
                if(receivedData == "SUCCESS"){
                   // cout << " ### HEARTBEAT: Tracking Server works normally" << endl;
                    pstru->_this->LoadGlobalNodeInfo();
                } else {
                    cout << " ### HEARTBEAT: Tracking Server is Down: Reconnect at next ping" << endl;
                    pstru->_this->is_trackingServer_online = false;
                }
            }
        }
    }


}

void* Client::Client_communicate_Download(void * pData){
    
    communicate_data* c_data = nullptr; 
    communicate_data c_data_local;
    if(((communicate_data *) pData)->addr_back == "host")
    {  
        c_data = (communicate_data *) pData;
    }
    else
    {
        communicate_data*  communicate_test = reinterpret_cast <communicate_data *>(pData);
        c_data_local = *communicate_test;
        c_data = &c_data_local;
        
    }
   
    bool is_detach = c_data->is_detach;
    string request = c_data->request;
    string addr_back = c_data->addr_back;
    int port_back = c_data->port_back;

    if(is_detach){
        pthread_detach(pthread_self());
    }

    Client* pThis;
    if(pThis = static_cast<Client *>(c_data->_this)){
        string receivedData = pThis->ForwardDownload(request);

        if(addr_back != "host"){
            int node_back = c_data->_this->GetAddrNode(addr_back, c_data->_this->global_nodeInfoList);
            int latency = c_data->_this->localLatencyGraph.getPathLength(node_back, c_data->_this->nodeID);

            sleep(latency/1000);

            UDP_send(addr_back.c_str(), port_back, receivedData.c_str());
            c_data->result = "";
        } else {
            c_data->result = receivedData;
        }

    }
    pthread_exit(NULL);
}




// debug

void Client::Debug(string filename){

    if(global_nodeInfoList.size() == 0){
        cout << "Not Enough Nodeinfo: Wait for auto-fetch from Server" << endl;
        return;
    }

    for(list<ServerInfo>::iterator it = global_nodeInfoList.begin(); it != global_nodeInfoList.end();it++){
        if(it->id == this->nodeID){
            continue;
        }
        cout << "Request node " << it->id << " to download " << filename << endl;
        Send("DEBUG",it->addr,PORT_CLIENT_DEBUG,PORT_CLIENT_FIND_RETURN,filename); // no return value
    }
    
    // self download at last
    cout << "Request node " << this->nodeID << " to download " << filename << endl;




    // Only record local time
    clock_t t1, t2;
    int times = 10;
    while(times > 0){
        t1 = clock();
        Download(filename);
        t2 = clock();

        ofstream outfile;
        outfile.open("time.txt", ios::app);

        outfile << global_nodeInfoList.size() << " "<<t2-t1 << endl;
        outfile.close();
        times -= 1;
    }

}






// Functionality


void Client::SetServerInfo(string addr, int id){
    this->trackingServerInfo.addr = addr;
    this->trackingServerInfo.id = id;
}



void Client::LoadLatencyList(string filename){

    // empty graph first
    this->localLatencyGraph.empty();

    ifstream infile;
    infile.open(filename.c_str());
    string line;
    while(getline(infile, line)){
        // split and load
        CStringTokenizer lineTokenizer;
        lineTokenizer.Split(line, " ");

        int node_1 = stoi(lineTokenizer.GetNext());
        int node_2 = stoi(lineTokenizer.GetNext());
        int latency = stoi(lineTokenizer.GetNext());

        // load
        this->localLatencyGraph.addEdge(node_1, node_2, latency);

    }
    
}

int Client::GetLatency(int id_1, int id_2, vector<ServerInfo> &forwardList){

    int end = id_2;
    int src = id_1;
    this->localLatencyGraph.shortestPath(id_1, this->dist);

    //print
    //cout << "--- Shortest Path Summary ---" << endl;

    if(this->dist[end] > MAX_LATENCY){
        //cout << "Unreachable within max latency to Node " << end << endl;
        return -1;
    }  
    for(int j=0;this->localLatencyGraph.path[src][end][j]!=-1;j++)
    {
        int id = this->localLatencyGraph.path[src][end][j];
        //cout<< "->"<< id;
        ServerInfo newNode;
        newNode.id = id;
        forwardList.push_back(newNode);
    }
    //cout<<endl;
    //cout << "Shortest path to Node " << end << " is: " << this->dist[end] << endl;

    return this->dist[end];
}



void Client::LoadFileList(string directory){


    this->sharedDirectory = directory;

    DIR *dir;
    struct dirent *entry;

    if( (dir = opendir(directory.c_str())) != NULL ){

        //cout << "Start loading..." << endl;
        // empty local list first
        localFileList.clear();
        while( (entry = readdir(dir)) != NULL){

            // filter . and .. (necessary!)
            if(!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, ".")){
                continue;
            } else {
                //cout << "---> Load File: " << string(entry->d_name) << endl;

                // load file list (notice add path for share/)
                FileInfo newFileInfo;
                strcpy(newFileInfo.filename, entry->d_name); // same size for 256
                newFileInfo.checksum = GetMD5("share/" +  string(newFileInfo.filename));
                localFileList.push_back(newFileInfo);
            }
        }
        closedir(dir);
        //cout << "End loading..." << endl;
    } else {
        // can not read directory
        perror("cant open directory: \n");
    }

}

void Client::PrintFileList(){
    list<FileInfo>::iterator it = localFileList.begin();
    if(localFileList.begin() == localFileList.end()){
        cout << "Empty Local File List" << endl;
    } else {
        cout << "--- Local File List ---" << endl;
        // print them all
        for(it = localFileList.begin(); it != localFileList.end(); it++){
            cout << "   -> " << it->filename << "   checksum: " << it->checksum << endl;
            
        }
    }
}


string Client::PackFileInfo(string filename){       
    string content = File2String(filename);
    if(content == ""){
        return "";
    }
    string checksum = GetMD5(filename);
    string final_content = checksum + content;
    //cout << "Final Packed Content: " << final_content << endl;
    return final_content;
}


FileInfo Client::UnpackFileInfo(string filename, string &content){
    string checksum = content.substr(0,32);
    content.erase(0,32);
    FileInfo newFileInfo;
    newFileInfo.checksum = checksum;
    memcpy(newFileInfo.filename, filename.c_str(), sizeof(filename));
    return newFileInfo;

}





string Client::File2String(string filename){
    // read file to const string (char)
    ifstream infile;
    infile.open(filename.c_str());

    if(infile.fail()){
        cout << "Error: can not find file " << filename << " locally" << endl;
        return "";
    }


    infile.seekg(0,ios::end);
    size_t length = infile.tellg();

    //cout << "   File Size (in File2String): " << length << endl;
    if(length >= MAX_FILESIZE){
        perror("length larger than max filesize, please check diretory or file\n");
        return "";
    }

    // set position back
    infile.seekg(0);
    string content(length + 1, '\0');
    infile.read(&content[0], length);
    infile.close();

    return content;

}

void Client::String2File(string filename, string content){

    ofstream outfile;
    outfile.open(filename.c_str(), ios::trunc);
    outfile << content;
    outfile.close();

}


string Client::GetMD5(string filename){


    string content = File2String(filename);

    // get MD5 value 
    const char* fileContent = content.c_str();
    //cout << "   [" << fileContent << "]" << endl;  // print content 
    MD5_CTX context;

    unsigned char MD5Value[MD5_SIZE];

    MD5_Init(&context);
    MD5_Update(&context, fileContent, strlen(fileContent));
    MD5_Final(MD5Value, &context);

    char MD5_char[32];
    string MD5_string;
    // print out the md5 value
    //cout << "   MD5: ";
    for(int i = 0; i < MD5_SIZE;i++){
        sprintf(MD5_char,  "%02x", MD5Value[i]);
        MD5_string.append(string(MD5_char));
    }
    //cout << MD5_string << endl;
    return MD5_string;
        
}

bool Client::CompareMD5(string MD5_value, string content){

    // get MD5 value 
    const char* fileContent = content.c_str();
    //cout << "   [" << fileContent << "]" << endl;  // print content 
    MD5_CTX context;

    unsigned char MD5Value[MD5_SIZE];

    MD5_Init(&context);
    MD5_Update(&context, fileContent, strlen(fileContent));
    MD5_Final(MD5Value, &context);

    char MD5_char[32];
    string MD5_string;
    // print out the md5 value
    //cout << "   MD5: ";
    for(int i = 0; i < MD5_SIZE;i++){
        sprintf(MD5_char,  "%02x", MD5Value[i]);
        MD5_string.append(string(MD5_char));
    }
    //cout << MD5_string << endl;
    
    // compare
    if(MD5_value== MD5_string){
        return true;
    } else {
        return false;
    }
        
}






// ---- Graph Implementation ----

Graph::Graph(int n):V(n)
{
    this->adj = new list<ipair>[n];
    this->path = new vector<vector<int>>[n];
    for(int i=0;i<n;++i)
    {
        path[i].resize(n,vector<int>(n,-1));
    }
};

void Graph::empty(){
    adj->empty();
    path->empty();
}


void Graph::addEdge(int u, int v, int w) 
{ 
    adj[u].push_back(make_pair(v, w)); 
    adj[v].push_back(make_pair(u, w)); 
} 

int Graph::getPathLength(int u, int v){
    list<ipair>::iterator i;
    for(i = adj[u].begin(); i != adj[u].end();i++){
        if(i->first == v){
            return i->second;
        }
    }
    cout << "No path (" << u << " - " << v << ") founded" << endl;
    return -1; 
}



void Graph::shortestPath(int src, vector<int>& dist)//int end,vector<ServerInfo> &forwardList
{
    priority_queue<ipair,vector<ipair>,cmp> pq;
    dist.clear(); 
    //vector<int> dist(V,INT_MAX);
    dist.insert(dist.begin(),V,INT_MAX);

    pq.push(make_pair(src, 0));  
    dist[src] =0;

    vector<int> index(V,0); // index for path
    while(!pq.empty())
    {
        int u = pq.top().first; // least distance vertex
        pq.pop();

        list<ipair>:: iterator i;

        for(i = adj[u].begin();i !=adj[u].end();++i)
        {
            int v = (*i).first;
            int weight = (*i).second;

            if(dist[v]>dist[u]+weight)
            {
                //cout<<v<<" "<<u<<endl;
                dist[v] = dist[u]+weight;
                //fix path
                int k=0;
                for(;path[src][u][k]!=-1;k++)//copy path u
                {
                    path[src][v][k] = path[src][u][k];
                }
                path[src][v][k++] = v;
                path[src][v][k] = -1;
                pq.push(make_pair(v,dist[v]));
            }
        }

    }

}





// ------ Main ------


int main(int argc, char *argv[]) {

    cout << "Start Main" << endl;
    srand( (unsigned)time( NULL ));

    bool Is_live= true;
    struct mypara pstru;
    Client* node;


    // thread
    pthread_t listen_getload_thread;
    pthread_t listen_download_thread;
    pthread_t listen_debug_thread;
    pthread_t heartbeat_thread;
    
    node = new Client();

    if (argc < 3) {
        fprintf(stderr, "usage: %s host message nodeID\n", argv[0]);
        exit(-1);
    }
    node->StartUp(string(argv[1]), stoi(argv[2]), "share/", "latency.txt"); // unique id for tracking server (also unused)
    
    
    
    pstru._this = node;
    pstru.Is_live = &Is_live;
    pthread_mutex_init(&(node->client_mutex), NULL);
    pthread_create(&listen_getload_thread, NULL, Client::Client_listen_Getload, (void*)&pstru);
    pthread_create(&listen_download_thread, NULL, Client::Client_listen_Download, (void*)&pstru);
    pthread_create(&listen_debug_thread, NULL, Client::Client_listen_Debug, (void*)&pstru);

    pthread_create(&heartbeat_thread, NULL, Client::Client_heartbeat, (void*)&pstru);

    system("clear");

    // test

    // cout << "Path length: " << node->localLatencyGraph.getPathLength(0,1) << endl;
    // cout << "Path length: " << node->localLatencyGraph.getPathLength(0,2) << endl;
    // cout << "Path length: " << node->localLatencyGraph.getPathLength(1,2) << endl;
    // cout << "Path length: " << node->localLatencyGraph.getPathLength(1,3) << endl;


    // UI input 
    char buffer[BUFLEN];

    while(1){
		printf("------\nCommand:\ndownload / updatelist / getload / exit / find / debug \n");/// synch
		std::cin.getline(buffer, BUFLEN);
		if(strcmp(buffer,"download") == 0){
            cout << "Input filename: ";
            cin.getline(buffer, BUFLEN);
            node->Download(string(buffer));
           

		} else if (strcmp(buffer,"updatelist") == 0){
            node->UpdateList();

		} else if(strcmp(buffer,"getload") == 0){
            cout << "Input node ID: ";
            cin.getline(buffer, BUFLEN);
            cout << "Load: " << node->GetLoad(stoi(buffer)) << endl;

		} else if(strcmp(buffer,"exit") == 0){
            break;

		} else if(strcmp(buffer,"find") == 0){
            cout << "Input filename: ";
            cin.getline(buffer, BUFLEN);
            node->Find(string(buffer));

		} else if(strcmp(buffer,"debug") == 0){
            cout << "Input filename: ";
            cin.getline(buffer, BUFLEN);
            node->Debug(string(buffer));
            
  
		} else {
			system(buffer);
		}
	}
   
    int nRes1 = pthread_join(listen_getload_thread, NULL);
    int nRes2 = pthread_join(listen_download_thread, NULL);
    int nRes3 = pthread_join(listen_debug_thread, NULL);
    
    int nRes = pthread_join(heartbeat_thread, NULL);
    pthread_mutex_destroy(&(node->client_mutex));

    return 0;
    
   
    // test
    /*
    
    unsigned char MD5Value1[16]; 
    unsigned char MD5Value2[16]; 
    unsigned char MD5Value3[16]; 

    client.GetMD5("share/test.txt", MD5Value1);
    client.GetMD5("share/test2.txt", MD5Value2);
    client.GetMD5("share/test3.txt", MD5Value3);

    cout << client.CompareMD5(MD5Value1,MD5Value2) << endl; // should be false
    cout << client.CompareMD5(MD5Value1,MD5Value3) << endl; // should be true (for value)

    string test = client.File2String("share/test.txt");
    client.String2File("share/writetest.txt", test);

    client.LoadFileList("share");
    client.PrintFileList();


    client.LoadLatencyList("latency.txt");
    int l = client.GetLatency(1,3);
    cout << "Latency: " << l << endl;

    list<ServerInfo> testNodeInfoList;
    client.LoadServerInfo("1;127.0.0.1;2;123.2.2.3", testNodeInfoList);

    for(list<ServerInfo>::iterator it = testNodeInfoList.begin(); it != testNodeInfoList.end(); it++){
         cout << it->id << endl;
         cout << it->addr << endl;

    }
    cout << client.PackFilelistInfo() << endl;

    */



}