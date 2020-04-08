
#include <iostream>
#include <fstream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <queue>

#include "BB_system.h"

#define PRIMARYPORT 5100
#define BACKUPPORT 5101

#define FORWARDPORT 6101
#define REQUESTPORT 6102
#define GETPORT 6103
#define OPERATIONPORT 6104
#define RESULTPORT 6105
#define HEARTPORT 6106

#define GETNEWVER_PORT 7001
#define WRITESELECT_PORT 7002
#define SYNCH_PORT 7003
#define REPLY_PORT 7004
#define HEARTBEAT_PORT 7005


class seq_server: public Server
{
    bool update_backup(string request); // primary send request to backup to update
    void forwardW2(string request);
    void forward_update(string request,int req_num=0); // for primary
    void Post_handler(const SServerInfo&  client_addr, string request);
    void Read_handler(const SServerInfo&  client_addr); 
    void Choose_handler(const SServerInfo&  client_addr, string request); 
    void Reply_handler(const SServerInfo&  client_addr, string request);
    static queue<string> requestQ;  // for primary  [COMMAND_TYPE;request;requestID]
    static int ID; //ariticle ID
    
public:
   seq_server(bool Coord = false);
   void* primary_backup_listenImpl(void * pData);
   static void* primary_backup_listen(void * pData);
};

class qum_server: public Server
{

    void Post_handler(const SServerInfo&  client_addr, string request);
    void Read_handler(const SServerInfo&  client_addr); 
    void Choose_handler(const SServerInfo&  client_addr, string request); 
    void Reply_handler(const SServerInfo&  client_addr, string request);
    bool loadArticle(string content);
    

    bool synchSelectedServer(vector<SServerInfo> serverSelected, string content);

    vector<SServerInfo> selectRandomServer(int size, int select, vector<SServerInfo> IPs);
    vector<SServerInfo>::iterator findServerByIndex(int index, vector<SServerInfo> IPs);
    string getNewestServer(vector<SServerInfo> serverSelected);
    bool writeSelectedServer(vector<SServerInfo> serverSelected, string content, int preHie);
    void writeArticle(string content);
    void writeArticle(string content, int preHierarchy);
    
    bool isValidQR(int N, int Nr, int Nw);
    bool isValidQW(int N, int Nr, int Nw);
    bool isValidQ(int N, int Nr, int Nw);

    string generatePrefix(const SServerInfo client_addr, string request, string TYPE, int preHie);

    void FORWARD_handler(string request);
    //static queue<string> requestQ;  // for primary  [COMMAND_TYPE;request;requestID]

    int ID = 0;
    int VER = 0;

    int N;
    int Nr;
    int Nw;
    
public:
   qum_server(bool Coord = false);
   void* quorum_listenImpl(void * pData);
   static void* quorum_listen(void * pData);
   void* heartBeatImpl(void * pData);
   static void* quorum_heartbeat(void * pData);
};



class LocalWrite_server: public Server
{
    bool update_backup(string request); // primary send request to backup to update
    bool update_local(string request,int articleID);
    void MoveItemX(const string& curIP);//W2
    void forward_update(string request,int articeID, int req_num=0); // for primary
    void Post_handler(const SServerInfo&  client_addr, string request);
    void Read_handler(const SServerInfo&  client_addr); 
    void Choose_handler(const SServerInfo&  client_addr, string request); 
    void Reply_handler(const SServerInfo&  client_addr, string request);
    static queue<string> requestQ;  // for primary  [COMMAND_TYPE;request;requestID]
    
public:
   LocalWrite_server(bool Coord = false);
   void* LocalWrite_listenImpl(void * pData);
   static void* LocalWrite_listen(void * pData);
};
