#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include<vector>
#include<string>

using namespace std;

#define SERVERPORT 5105  //Port for server
#define CLIENTPORT 5106  //port for client
const int N = 10;




// struct SServerInfo
// {
// 	char m_szIPAddress[16];
// 	int m_nPort;
// };

struct Article
{
    unsigned int ID;
    char* primaryIP;
    vector<string> content;//[Version;Title;author;Time;content]$
    
    unsigned int hierachy;// how many space when reading
};

// a client can get access to all the server
struct ServerInfo{
    string addr;
    int port;
};

struct LocalArticle{
    //[Version;Title;author;Time;content;hierachy]
    int id;
    int hierachy;
    string version;
    string title;
    string author;
    string time;
    string content;
};

class Client
{
private:
    list<ServerInfo> serverList;
    list<LocalArticle> articleList;
    vector<char*> IP; 
    void LoadIPs(string fileName);
public:
    Client();//Add IP in constructor
    void writeServer(char* host, int port, string filename);
    void printServerInfo();
    void printServerInfo(list<ServerInfo>::iterator it);
    void readServer(string filename);
    list<ServerInfo>::iterator pickRandomServer();
    list<ServerInfo>::iterator pickRandomServer(int index);
    int loadReadContent(const char buffer[]);
    void printReadContent(int start, int len);
    void printReadContent(int page);
    bool communicate_request(const char* host, unsigned short port, const char* command, char *buffer, bool needFeedBack);
    int articleToPage(int articleNum);
    //void Synch();
    void Post(string request) ;//[POST;Version;Title;author;Time;content]
    int Read();  //[READ]
    void Choose(unsigned int ID); // [CHOOSE;ID]
    void Reply(unsigned int ID, string request);//[REPLY;ID;Version;Title;author;Time;content]
    void Synch();
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
// vector<SServerInfo>::iterator findIPs(const string strIPAddress, int nPort, vector<SServerInfo>& IPs);
// void removeIP(string strIPAddress,int nPortconst,  vector<SServerInfo>& IPs);

