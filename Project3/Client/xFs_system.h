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

using namespace std;

#define BUFLEN 65536


#define PORT_SERVER_UPDATELIST 6000 // listen from client for updatelist
#define PORT_SERVER_FIND 6001 // list from client for find
#define PORT_SERVER_HEARTBEAT 6002

#define PORT_CLIENT_DOWNLOAD 6000  // listen for download request
#define PORT_CLIENT_GETLOAD 6001  // listen for getload request
#define PORT_CLIENT_GETLOAD_RETURN 6002 // listen for getload result
#define PORT_CLIENT_FIND_RETURN 6003 // list for find result
#define PORT_CLIENT_UPLOADLIST_RETURN 6004 // listen for updatelist result
#define PORT_CLIENT_HEARTBEAT_RETURN 6005
#define PORT_CLIENT_DEBUG 6006


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


