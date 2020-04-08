
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


#include "BB_system.h"
#include "stringTokenizer.h"

#define BUFLEN 8000    // standard size of buffer 8000
#define WAIT_TIME 3   // max wait time for feedback of request
#define ARTICLE_DISPLAY_LEN 50
#define PAGESIZE 3

char INPUT_BUFFER[BUFLEN];
char READ_BUFFER[BUFLEN];

using namespace std;

Client::Client()
{

}
void Client::writeServer(char* host, int port, string filename){
    // send request to ask for server info
    printf("Connect to ip %s with Port %d\n", host, port);
    // fill request data
    char command[BUFLEN];  // command
    string ip = checkip();
    cout<<ip<<endl;
    sprintf(command,"GETLIST;%s;%d;", ip.c_str(),port);

    cout << "--->Command Content: " << command << endl;

    string content = UDP_send_receive(host, SERVERPORT, command);

    CStringTokenizer contentTokenizer;
    contentTokenizer.Split(content, ";"); // TODO: change to what we need
    int contentLen = contentTokenizer.GetSize();
    cout << contentLen << endl;
    int contentNum = (contentLen+1)/2;
    cout << "Total Available Server Num: " << contentNum << endl;

    ofstream outfile;
    outfile.open(filename.c_str(), ios::trunc);
    

    for(int i = 0; i < contentNum; i++){
        // write article in buffer
        string addr = contentTokenizer.GetNext();
        cout << addr << endl;
        string port = contentTokenizer.GetNext();
        cout << port << endl;
        outfile << addr << " " << port << endl;;
        
    }
    
    outfile.close();

}


// only for testing
void Client::printServerInfo(){

    cout << "---Local Server List---" << endl;
    int count = 0;
    list<ServerInfo>::iterator it;
    for(it = serverList.begin(); it != serverList.end(); it++){
        count++;
        cout << count <<  "   " << it->addr << "  " << it->port << endl;
    }
    cout << "----------------------" << endl;
}


void Client::printServerInfo(list<ServerInfo>::iterator it){
    cout << "-> Select Server: " <<  "   " << it->addr << "  " << it->port << endl;
}

void Client::readServer(string filename){
    ifstream infile;
    infile.open(filename.c_str());
    string line;
    while(getline(infile, line)){
        istringstream ss(line);
        ServerInfo info;
        string s_port, s_addr;
        
        // load server info
        ss >> info.addr;
        ss >> s_port;
        info.port = stoi(s_port);

        serverList.push_back(info);
        //cout << info.addr << " and " << info.port << endl;
        //printServerInfo();
    }
}   


list<ServerInfo>::iterator Client::pickRandomServer(){
    if(serverList.size() == 0){
        cout << "no server selected: empty serverlist";
        return serverList.begin();
    }
    // randomly pick a server from server list
    list<ServerInfo>::iterator it = serverList.begin();
    srand((unsigned)time(NULL));
    int random = (rand() % (serverList.size()));
    advance(it, random);
    return it;
}


list<ServerInfo>::iterator Client::pickRandomServer(int index){
    // pick the server of index
    if(serverList.size() == 0){
        cout << "no server selected: empty serverlist";
        return serverList.begin();
    }
    list<ServerInfo>::iterator it = serverList.begin();
    advance(it, index);
    return it;
}


int Client::loadReadContent(const char buffer[]){ // print all content
    // assume that the read buffer store all articles
    // articles are divided by &, and the content are divided by ;
    // [Version;Title;author;Time;content;hierachy]

    cout << "--------Load Content--------" << endl;


    //empty article list first
    articleList.clear();

    
    CStringTokenizer bufferTokenizer;
    CStringTokenizer articleTokenizer;

    bufferTokenizer.Split(buffer, "$");
    int bufferLen = bufferTokenizer.GetSize();
    cout << "Total Article Number: " << bufferLen << endl;
    for(int i = 0; i < bufferLen;i++){
        // display the article i+1
        //cout << "loading article " << i+1 << endl;
        LocalArticle newArticle;

        string article = bufferTokenizer.GetNext();  //cout << "   new article: " << article << endl;
        articleTokenizer.Split(article, ";");

        // fit content

        newArticle.id = i+1;
        
        string version = articleTokenizer.GetNext();
        newArticle.version = version; //cout << "   version: " << version << endl;

        string title = articleTokenizer.GetNext();
        newArticle.title = title; //cout << "   title: " << title << endl;

        string author = articleTokenizer.GetNext();
        newArticle.author = author; //cout << "   author: " << author << endl;

        string time = articleTokenizer.GetNext();
        newArticle.time = time; //cout << "   time: " << time << endl;   
            
        string content = articleTokenizer.GetNext();
        newArticle.content = content; //cout << "   content: " << content << endl;

        string hierachy = articleTokenizer.GetNext();
        newArticle.hierachy = stoi(hierachy); //cout << "   hierachy: " << hierachy << endl;

        articleList.push_back(newArticle);


    }

    cout << "------End Load Content------" << endl << endl;
    return bufferLen;

}

void Client::printReadContent(int start, int len){
    if(articleList.size() == 0){
        loadReadContent(READ_BUFFER);
    }
    if(articleList.size() == 0){
        // something wrong with read buffer
        perror("READ buffer error");
        return;
    }
    // clear screen
    system("clear");
    cout << "------     Article List: Page " << div(start, PAGESIZE).quot+1 << "     -----" << endl;
    cout << endl << endl;
    // print content with size
    list<LocalArticle>::iterator it = articleList.begin();
    advance(it, start-1);

    if(it == articleList.end()){
        cout << "--> END" << endl;
    }

    for(int i = 0;i<len;i++){
        // print article
        // print id
        cout << it->id;
        // print space
        int space = it->hierachy;
        while(space > 0){
            cout << "    ";
            space -= 1;
        }       
        // print rest contnet
        cout << "   --" << it->title << "--(" << it->time << "  by " << it->author << 
        "): " << it->content.substr(0,ARTICLE_DISPLAY_LEN) << "..." << endl;

        // forward
        it++;
        if(it == articleList.end()){
            cout << "--> END"  << endl;
            break;
        }
    }
    cout << endl << endl;  
    
}

void Client::printReadContent(int page){
    int start = ((page-1)*PAGESIZE+1);
    int len = PAGESIZE;

    cout << "Page " << page <<" To Range: " << start << "   " << len << endl;
    printReadContent(start, len);
}


bool Client::communicate_request(const char* host, unsigned short port, const char* command, char *buffer, bool needFeedBack){

   // cout << "---- Start Communication ----" << endl;
    int listenfd = 0,n =0;
    socklen_t nClientAddr = 0;
	struct sockaddr_in  client_addr;

   // printf(" Client: Sent Command to Server. \n	(--->Command Content: %s)\n",command); 
    UDP_send(host,port,command);

	listenfd = UDP_bind(CLIENTPORT);
    n = recvfrom(listenfd, buffer, BUFLEN, 0, (struct sockaddr *)&client_addr, &nClientAddr);
    if(n==-1)
	{
		perror("Error: Fail to find register server\n");
		exit(1);
	} 
	buffer[n] = '\0'; 
	//printf("	Client: Received Feedback from Server: %s\n", buffer);
    close(listenfd);
    //cout << "---- End  Communication ----" << endl;
    if(!needFeedBack)
    {
        return true;
    } else if(needFeedBack)
    {
        // validate if succeed
        cout <<"   Ready to cut the buffer" << endl;
        CStringTokenizer bufferTokenizer;
        bufferTokenizer.Split(buffer, ";");
        string message = bufferTokenizer.GetNext();
        cout << "   Message: " << message << endl;
        string result = bufferTokenizer.GetNext();
        cout << "   Result " << result << endl;
        if(stoi(result) == 1)
        {
            cout << "success opeartion with feedback info: " << message << endl;
            return true;
        } else if (stoi(result) == 0)
        {
            cout << "uncessess operation with feedback info; " << message << endl;
            return false;
        }
    
    }
}

int Client::articleToPage(int articleNum){
    int pageNum;
    if(articleNum % PAGESIZE == 0){
        pageNum =  div(articleNum, PAGESIZE).quot;
    } else {
        pageNum =  div(articleNum, PAGESIZE).quot+1;
    }
    cout << "Article " << articleNum << "to Page: " << pageNum << endl;
    return pageNum;
}



int Client::Read(){

    cout << "---> Start Read" << endl;
    // select random server
    list<ServerInfo>::iterator it = pickRandomServer();
    const char* host = it->addr.c_str();
    int port = it->port;
    string localip = checkip();

    char command[BUFLEN];  // command

    // clear read buffer before new read
    memset(READ_BUFFER, 0, sizeof READ_BUFFER);

    // char content[BUFLEN];  // receive info
    snprintf(command,sizeof(command),"%s;%s;%d","READ",localip.c_str(), CLIENTPORT);
    communicate_request(host, port, command, READ_BUFFER, false);

    // deal with data
    int articleNum = loadReadContent(READ_BUFFER);
    //printReadContent(1);  // start from page one by default
    cout << "---> End Read" << endl;
    return articleNum;

}

 void Client::Synch(){

//     // using CHOOSE to send

     cout << "---> Start Synch" << endl;
     // select random server
     list<ServerInfo>::iterator it = pickRandomServer();
     const char* host = it->addr.c_str();
     int port = it->port;
     string localip = checkip();

     char command[BUFLEN];  // command

     // clear read buffer before new read
     memset(READ_BUFFER, 0, sizeof READ_BUFFER);

     // char content[BUFLEN];  // receive info
     snprintf(command,sizeof(command),"%s;%s;%d","CHOOSE",localip.c_str(), CLIENTPORT);
     communicate_request(host, port, command, READ_BUFFER, false);

     // deal with data
     //printReadContent(1);  // start from page one by default
     cout << "---> End Synch" << endl;

 }


    
void Client::Post(string request){

    cout << "---> Start Post" << endl;
    // select random server
    list<ServerInfo>::iterator it = pickRandomServer();
    const char* host = it->addr.c_str();
    int port = it->port;
    string localip = checkip();

    // fill request content
    char command[BUFLEN];
    char content[BUFLEN + 1];  // receive info

    snprintf(command,sizeof(command),"%s;%s;%d;%s","POST",localip.c_str(), CLIENTPORT, const_cast<char*>(request.c_str()));
    //snprintf(command,sizeof(command),"%s;%s;%s;%d","GETLIST","RPC",checkip(),port);
	communicate_request(host, port, command, content, true);
    cout << "---> End Post" << endl;


}

void Client::Choose(unsigned int ID){

    // if article already stage locally, should I update the content? I should

    cout << "---> Start Choose" << endl;
    if(articleList.empty()){
        // reload article
        Read();
        system("clear"); 
    }
    list<LocalArticle>::iterator it = articleList.begin();
    while(it != articleList.end()){
        if(it->id == ID){
            // display the article content
            system("clear");

            cout << "----------------------->" <<endl;
            cout << "Title: [" << it->title << "]" << endl;
            cout << "Time: " << it->time << endl;
            cout << "Author: " << it->author << endl;
            cout << it->content << endl;
            cout << "----------------------->" <<endl;
            return;
        }
        it++;
    }

    // if here, no id matched.
    cout << "No id matched article found" << endl;
    cout << "---> End Choose" << endl;
    return;
    

}


void Client::Reply(unsigned id, string request){
    cout << "---> Start Reply" << endl;
    // select random server
    list<ServerInfo>::iterator it = pickRandomServer();
    const char* host = it->addr.c_str();
    int port = it->port;
    string localip = checkip();

    // fill request content
    char command[BUFLEN];
    char content[BUFLEN];

    snprintf(command,sizeof(command),"%s;%s;%d;%d;%s","REPLY",localip.c_str(), CLIENTPORT, id, const_cast<char*>(request.c_str()));
    bool result = communicate_request(host, port, command, content, true);
    cout << "---> End Reply" << endl;

}

string GetGlobleTime()
{
    
   time_t now = time(0);

   // to char
   char* dt = ctime(&now);

   // to tm structure
   tm *gmtm = gmtime(&now);
   dt = asctime(gmtm);
   cout<<dt<<endl;
   string strTime = dt;
   strTime = strTime.substr(0,strTime.length()-1);
   return strTime;//format:Sun;Jan;9;03:07:41;2011
}

void testOperation(string Operation, string request, int & Entry, Client *clnt){
// record method usage and time comume for times            
    clock_t t1, t2;
    int f;


    // consider Operation type
    if (Operation == "READ"){
        // Entry = pageNum
        cout << "-------------------------Start->" << Operation << endl;
        t1 = clock();
        Entry = clnt->Read();
        t2 = clock(); 
        cout << "-------------------------End->" << Operation << endl;
    } else if (Operation == "POST"){
        cout << "-------------------------Start->" << Operation << endl;
        t1 = clock();
        clnt->Post(request);
        t2 = clock(); 
        cout << "-------------------------End->" << Operation << endl;
    } else if (Operation == "CHOOSE"){
        // Entry = ID
        cout << "-------------------------Start->" << Operation << endl;
        t1 = clock();
        clnt->Choose(Entry);
        t2 = clock(); 
        cout << "-------------------------End->" << Operation << endl;
    } else if (Operation == "REPLY"){
        cout << "-------------------------Start->" << Operation << endl;
        t1 = clock();
        clnt->Reply(Entry, request);
        t2 = clock(); 
        cout << "-------------------------End->" << Operation << endl;
    } else {
        cout << "Invalid Operation" << endl;
    }

    ofstream outfile;
    outfile.open("test.txt", ios::app);

    outfile << Operation << " " << t2-t1 << endl;
    cout << "Operation " << Operation << " Time: " << t2-t1 << endl;

    outfile.close();
}



// for testing
int main (int argc, char *argv[]){
    // read server for local test
    cout << "Start Main" << endl;
    clock_t start, finish;
    double  duration; 
    char buffer[BUFLEN];
    bool isRead = false;
    int currentPage = 1;
    int articleNum = 0;
    int pageNum;
    string curtime;
    Client* clnt = new  Client();

    if(argc < 2){
        printf ("usage: %s registry_host\n", argv[0]);
		exit (1);
    } 
    clnt->writeServer(argv[1], SERVERPORT, "serverInfo.txt");

    // print all server for reference
    clnt->readServer("serverInfo.txt");
    clnt->printServerInfo();

    // test select random server
    cout << "[TEST]";
    list<ServerInfo>::iterator it = clnt->pickRandomServer(0);
    clnt->printServerInfo(it);

    system("clear");
    cout << "Entering Main" << endl;

    while(1){
		printf("------\nCommand:\nread / choose / post / reply / exit / test\n");/// synch
		std::cin.getline(buffer, BUFLEN);
		// read
		if(strcmp(buffer,"read") == 0){
            start = clock(); 
            articleNum= clnt->Read();
            pageNum = clnt->articleToPage(articleNum);
            cout << "Article Num: " << articleNum << endl;
            clnt->printReadContent(currentPage);
            finish = clock();    
            duration = ((double)(finish - start) / CLOCKS_PER_SEC)*1000.0;    
            printf( "%f milliseconds for read\n", duration );
		// choose
		} else if(strcmp(buffer,"d") == 0){
            currentPage = min(currentPage + 1, pageNum);
            clnt->printReadContent(currentPage);
		// choose
		} else if(strcmp(buffer,"a") == 0){

            currentPage = max(currentPage - 1, 1);
            clnt->printReadContent(currentPage);
		// choose
		} else if (strcmp(buffer,"choose") == 0){
            // TODO: only for testing!!!
            cout << "Input test target article ID" << endl;
            cin.getline(buffer, BUFLEN);
            clnt->Choose(stoi(buffer));
		// post
		} else if(strcmp(buffer,"post") == 0){
            string article;
            string request = "1;";//[Version]
            cout<< "please input the article:\nformat:[Autor;Title;content]\n"<<endl;
            cin >> article;
            CStringTokenizer stringTokenizer;
            stringTokenizer.Split(article, ";");
            string autor = stringTokenizer.GetNext(); 
            string title = stringTokenizer.GetNext();
            string content = stringTokenizer.GetNext();
            if(autor.empty()||autor.empty()||content.empty()||!stringTokenizer.GetNext().empty())
            {
                cerr<<"Undefined input format!!"<<endl;
                continue;
            }
            curtime = GetGlobleTime();
            request.append(title).append(";");
            request.append(autor).append(";");
            request.append(curtime).append(";");
            request.append(content);
            // test only here
           // Post("1;Test1;A;11:10;Test Content 1");
            start = clock();
            clnt->Post(request);
            finish = clock();    
            duration = ((double)(finish - start) / CLOCKS_PER_SEC)*1000.0;    
            printf( "%f milliseconds for post\n", duration );
		// reply
		} else if(strcmp(buffer,"reply") == 0){
            string article;
            string request = "1;";
            cout << "Input target article ID" << endl;
            cin.getline(buffer, BUFLEN);
            if(stoi(buffer)<0)
            {
                cerr<<"target article ID should >0 !"<<endl;
                continue;
            }
            cout<< "please input the article:\nformat:[Autor;Title;content]\n"<<endl;
            cin >> article;
            CStringTokenizer stringTokenizer;
            stringTokenizer.Split(article, ";");
            string autor = stringTokenizer.GetNext(); 
            string title = stringTokenizer.GetNext();
            string content = stringTokenizer.GetNext();
            curtime = GetGlobleTime();
            request.append(title).append(";");
            request.append(autor).append(";");
            request.append(curtime).append(";");
            request.append(content);
            if(autor.empty()||autor.empty()||content.empty()||!stringTokenizer.GetNext().empty())
            {
                cerr<<"Undefined input format!!"<<endl;
                continue;
            }
            start = clock();
            clnt->Reply(stoi(buffer),request);
            //Reply(stoi(buffer),"0;TestR;Reply;11:11;Test Reply Content 1");

            finish = clock();    
            duration = 1000.0*((double)(finish - start) / CLOCKS_PER_SEC);    
            printf( "%f milliseconds for reply\n", duration );

		//
		} else if(strcmp(buffer,"exit") == 0){
            break;
		//
		} 
        // else if(strcmp(buffer,"synch") == 0){
        //     Synch();
		// //
		// }
        else if(strcmp(buffer,"test") == 0){

            // record method usage and time comume for times      

            int ID = 1;      
            int DUMMY = 1;
            int interval = 2;

            int times = 5;


            for(int i = 0; i < times ;i++ ){
                ID = i;
                testOperation("POST","1;Title;Author;Time;Content",DUMMY,clnt);
                sleep(interval);
                testOperation("POST","2;Title;Author;Time;Content",DUMMY,clnt);
                sleep(interval);
                testOperation("READ","",articleNum,clnt);
                sleep(interval);
                testOperation("REPLY","3;Reply to 1;Author;Time;Content",ID,clnt);
                sleep(interval);
                testOperation("REPLY","4;Reply to 1;Author;Time;Content",ID,clnt);
                sleep(interval);
                testOperation("READ","",articleNum,clnt);
                sleep(interval);
            }
            


		//
		} else if(strcmp(buffer,"debug") == 0){
            // all test code
            char testBuffer[] = "1;Test1;Foursay;11:10;This is Conteant;0\
            $2;Test2;Inutto;12:10;This is Content 2, and TryingToReachTheMaxLen;4\
            $3;Test3;Foursay;11:10;This is Content;4$2;Test3;Foursay;11:13;This is Content 3;0\
            $2;Test4;Foursay;11:14;This is Content 4;0$4;Test5;Foursay;11:55;This is Content 5;4\
            $2;Test6;Foursay;11:67;This is Content 6;0";
            memcpy(READ_BUFFER, testBuffer,sizeof(testBuffer));
            articleNum =  clnt->loadReadContent(READ_BUFFER);
            pageNum = div(articleNum, PAGESIZE).quot+1; 
             clnt->printReadContent(1);  // read id size
		//
		}else {
			system(buffer);
		}
	}
    return 0;
}