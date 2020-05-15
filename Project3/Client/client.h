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
#include <memory>

using namespace std;

#define MAX_FILESIZE 65536 // 64 kb
#define MAX_NODENUM 30
#define MAX_LATENCY 200000

#define MD5_SIZE 16
#define MAX_FILENAME_SIZE 256

#define MAX_LOAD 4000

#define DOWNLOAD_POOL_START 5000
#define DOWNLOAD_POOL_END 5900

// for algorithm


struct ServerInfo{
    string addr;
    int id; 
    // int load;
};





typedef pair<int,int>  ipair;//(vertex weight)

class Graph{

    int V;// #vetices
    list<ipair> *adj; //edges matrix
    
    // vector<int> dist;

public:

    Graph(int V);
    vector<vector<int>> * path;
    void empty();
    void addEdge(int u, int v, int w);  // function to add an edge to graph 
    int getPathLength(int u, int v); 
    void shortestPath(int src, vector<int>& dist);  // Dijkstra int end, vector<ServerInfo> &forwardList
};

struct cmp
{
    bool operator()(ipair a,ipair b)
    {
        return (a.second >= b.second? true:false);
    }

};


struct FileInfo{

    char filename[MAX_FILENAME_SIZE]; // defined by dirent
    //unsigned char checksum[MD5_SIZE];
    string checksum;

};

// download listen request port: 6000
// upload<------->download :pool(5000-5900)
// Getload listen port:6001
// Getload return: 6002
// Find return: 6003 
// UploadList success return: 6004

struct communicate_data;




class Client
{
private:

    vector<int> dist;  // record min distance to other load
    list<FileInfo> localFileList; // store file info locally. report this content with node info.
    ServerInfo trackingServerInfo;
    int Load;
    list<int> downloadPortPool;
    list<communicate_data> c_dataList;

    list<ServerInfo> global_nodeInfoList;

    int nodeID;
    
   
    string sharedDirectory;


     // functionality
    void SetServerInfo(string addr, int port); //set tracking server location
    bool isValidServerInfo();

    //int GetLocalLoad();  // get load locally and return

    string PackFileInfo(string filename);
    FileInfo UnpackFileInfo(string filename, string &content);


    int GetPort();
    void ReturnPort(int listen_port);

    void LoadFileList(string directory);
    void PrintFileList();

    void LoadLatencyList(string filename); // load latency into graph
    int GetLatency(int id_1, int id_2, vector<ServerInfo> &forwardList); //temp use int... just for now

    void String2File(string filename, string content);
    string File2String(string filename); // return empty if error

    string GetMD5(string filename); // the size of MD5value must be 16 -> string: 32
    bool CompareMD5(string filename, string content); // filename -> local, content-> data
    void PrintMD5(unsigned char *value);
    

    // from Project 2: communication
    //bool communicate_request(const char* host, unsigned short port, const char* command, char *buffer, bool needFeedBack);

    
public:
    Client();
    pthread_mutex_t client_mutex;
    Graph localLatencyGraph = Graph(MAX_NODENUM); // store latency graph
    bool is_trackingServer_online;


    // tools for core functions
    void LoadServerInfo(string receivedData, list<ServerInfo> &nodeInfoList);

    string Communicate(string operationName, string serverAddr, int serverPort, int clientPort, string request);
    void Send(string operationName, string serverAddr, int serverPort, int clientPort, string request);

    string PackFilelistInfo();
    string GetNodeAddr(int nodeID, list<ServerInfo> nodeInfoList);
    int GetAddrNode(string Addr, list<ServerInfo> nodeInfoList);
    
    void LoadInfoListByID(vector<ServerInfo> &forwardList, list<ServerInfo> nodeInfoList);
    int GetMinLoadNode(list<ServerInfo> nodeInfoList);
    void RemoveNodeInfo(int nodeID, list<ServerInfo> &nodeInfoList);

    void FillForwardList(vector<ServerInfo> &forwardList, list<ServerInfo> nodeInfoList, int minload_id);
    string ForwardDownload(string &request);
    string GetNextForwardAddr(string &request);
    string PackForwardRequest(vector<ServerInfo> forwardList, string filename);
    bool ValidatePackedData(string receivedData, string filename);

    void LoadGlobalNodeInfo();

    // Core part 
    void StartUp(string addr, int id, string sharedDirectory, string latencyFile);
    void Download(string filename); // FOrwardRequest: [addr1;addr2;addr3;$;content]



    
    void UpdateList();//[NODEID;filename;MD5;.....]
    int GetLoad(int nodeID, list<ServerInfo> nodeInfoList); // NO modification for nodelist so it should be fine
    int GetLoad(int nodeID);
    string Find(string filename);// server return [cln_id;cln_ip;....]  


    void Debug(string filename);
    
    // listen
    void * Client_listenImpl(void * pData, int listenPort);
    static void * Client_listen_Download(void * pData);
    static void * Client_listen_Getload(void * pData);
    static void * Client_listen_Debug(void * pData);
    
    static void * Client_communicate_Download(void * pData);

    static void * Client_download(void * pData);
    static void StartDownloadThread(string filename);
    
    static void * Client_heartbeat(void * pData);
    
};

struct mypara
{
    Client* _this;
    bool * Is_live;
    string filename;
}; 

struct communicate_data{
    Client* _this;
    string request;      // parameter
    string result;   // returnvalue

    string addr_back;
    int port_back;

    bool is_detach;
    communicate_data(){};
    communicate_data(const communicate_data& b);
    communicate_data operator =(const communicate_data& b);

};
