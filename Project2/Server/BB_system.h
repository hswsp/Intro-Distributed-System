#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include<vector>
#include<string>

using namespace std;

#define SERVERPORT 5105  //Port for server
#define CLIENTPORT 5106  //port for client
const int N = 10;

struct SServerInfo
{
	char m_szIPAddress[16];
	int m_nPort;
};

struct Article
{
    unsigned int ID;
    string primaryIP;
    vector<string> content;//[Version;Title;author;Time;content]$
    
    unsigned int hierachy;// how many space when reading
};

class Server
{
private:
    void addServer(string strIPAddress, int nPort);
    void addClient(string strIPAddress, int nPort);
    void synch();
   
    string getServerList();
    virtual void Post_handler(const SServerInfo& client_addr, string request) =0;
    virtual void Read_handler(const SServerInfo&  client_addr) =0; 
    virtual void Choose_handler(const SServerInfo&  client_addr, string request) =0; 
    virtual void Reply_handler(const SServerInfo&  client_addr,string request) =0;
    
public:

    string coordinatorIP; //one coordinator
    bool IsCoordinator;

    static vector<SServerInfo> SIPs;// all servers' IP， let assume only coordinator has it
    vector<SServerInfo> CIPs;// all clients' IP connected to this server
    vector<Article> articles;
    Server(bool Coord = false);
    int RegisterS(string coordinatorIP);
    void DeregisterS(bool* Is_live);
    void* listen(void * pData);// for coordinator
    void* Command(void * pData);
    static void* listenThread(void * pData);
    static void* CommandThread(void * pData);
    void  sendArticle(const char* targetIP,unsigned short targetport);//[Version;Title;author;Time;content;hierachy$]
    vector<Article>::iterator findArticlebyID(unsigned int ID);
};

struct mypara
{
    Server* _this;
    bool * Is_live;
}; 


int UDP_bind(unsigned short targetport);
int UDP_send(const char* target,unsigned short targetport, const char* message); // 0 for success, 1 for failure
string receive(unsigned short targetport);//recvfrom() shall block until a message arrives
string UDP_send_receive(const char* target,unsigned short targetport, const char* message);
string checkip(); //Get Local IP
bool isValidIP(const string ipAddress);//Check the sting whether it is vaild or not.
bool isValidPort(const int nPort);
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
string currentDateTime(); 
void   Delay(int   time);//time*1000 is second
vector<SServerInfo>::iterator findIPs(const string strIPAddress, int nPort, vector<SServerInfo>& IPs);
void removeIP(string strIPAddress,int nPortconst,  vector<SServerInfo>& IPs);

