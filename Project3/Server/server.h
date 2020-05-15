#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <vector>
#include <string>


#define MD5_SIZE 16

using namespace std;

struct FileInfo{

    char filename[256]; // defined by dirent
    //unsigned char checksum[MD5_SIZE];
    string checksum;
    FileInfo();
    FileInfo(string name, string ck);
    
};


// updatelist format: [UPDATELIST;cnt_ip;clnt_port;request]
// find format: [FIND;clnt_ip;clnt_port;request]
// updatelist listen port: 6000
// find listen port: 6001

#define FINDPORT 6001
#define UPDATEPORT 6000
#define HEARTBEATPORT 6002
struct ClientInfo
{
    int id;
    string host;
    int port;
    vector<FileInfo> fileList;
    ClientInfo(){};
    ClientInfo(const ClientInfo& si):id(si.id),host(si.host),port(si.port){};
    ClientInfo(int id_m,string host_m,int port_m):id(id_m),host(host_m),port(port_m){};
    bool operator == (const ClientInfo& si) 
    {
        return (si.host==this->host) && (si.port == this->port);
    }
    bool operator()(const ClientInfo& si) // for compare
    {
        return this->id==si.id &&this->host==si.host &&this->port==si.port;
    }
    bool fileMatch(const string& fname) const; // check whether file exists
};

class Server
{

    
    void FindResImp(void * pData,int port);
    void RecvListImp(void * pData,int port);
    void HeartbeatImp(void * pData,int port);//[HEARTBEAT;ip;port;""]
public:
    list<ClientInfo> clientInfoList;
    static void* RecvList(void * pData);
    static void* Heartbeat(void * pData);
    static void *FindRes_Getload(void * pData);

};
struct mypara
{
    Server* _this;
    bool * Is_live;
}; 

