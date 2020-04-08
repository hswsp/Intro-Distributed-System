#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <time.h> 
#include <stdlib.h> 
#include <sstream>
#include <iostream>
#include <algorithm>
#include "stringTokenizer.h"
#include "Server.h"
#define BUFLEN 8000	//Max length of buffer
using namespace std;
char *serverIP;
typedef enum {UNKNOWN_COMMAND=0, FORWARD, UPDATE,ACKUP,ACKCOMP} COMMAND_TYPE;
typedef enum {UNKNOWN_COMMAND_L=50, MOVEX,PRIMCHAN,FORWARD_L, UPDATE_L,ACKUP_L,ACKCOMP_L} COMMAND_TYPE_L;
typedef enum {UNKNOWN_COMMAND_QUM=10, FORWARD_QUM,RECEIVEVER, REQUESTVER, REQUESTART,OPERATION_READ,OPERATION_WRITE,OPERATION_REPLY, OPERATION_SYNCH} COMMAND_TYPE_QUM;

int seq_server::ID = 0;
queue<string> seq_server::requestQ;
queue<string> LocalWrite_server::requestQ;



seq_server:: seq_server(bool Coord):Server(Coord)
{

}

void seq_server::forward_update(string request,int req_num) //send W3
{
    vector<SServerInfo>::iterator it;

    requestQ.push(request); // push current back

    string commd = "update;";
    string localip = checkip();
    commd.append(localip).append(";");//primary  ip
    commd.append(to_string(PRIMARYPORT)).append(";");//port


    commd.append(std::to_string(req_num)).append(";"); 
    int articeID = ++seq_server::ID;
    commd.append(to_string(articeID)).append(";");//[update;primary ip;port;req_ID;article ID;repID; request]
    string req = requestQ.front();//pop head out
    requestQ.pop();

    commd.append(req);
    //cout<< "commd in forward_update: "<<commd<<endl;
    for(it=SIPs.begin();it!=SIPs.end();it++)//broad cast W3
    {
        UDP_send(it->m_szIPAddress,PRIMARYPORT, commd.c_str()); 
    }
    
    return;
    
}

bool seq_server::update_backup(string request)
{
    
    string localip = checkip();
    Article artic;
    //update storage
	CStringTokenizer stringTokenizer;
    stringTokenizer.Split(request, ";");//[req_ID;article ID; rep_ID; request]
    stringTokenizer.GetNext(); //req_ID
    int contentID = atoi(stringTokenizer.GetNext().c_str());//aritcle ID
    // cout<<"contentID is "<<contentID<<endl; 
    int repID = atoi(stringTokenizer.GetNext().c_str());//ID
    //cout<<"repID "<<repID<<endl;

    if(repID!= -1)
    {
        vector<Article>::iterator it = this->findArticlebyID(repID);
        if(it!=articles.end())
        {
            artic.hierachy = it->hierachy + 1;
        }
        else
        {
            perror("Not found article");
            return false;
        }
    }
    else
    {
        artic.hierachy = 0;
    }
    
    size_t  firstelement = request.find_first_of(";");//[req ID; content ID; rep_ID;  request]
    //cout<< "request "<<request<<endl;
    string content = request.substr(firstelement + 1);//[content ID; rep_ID;  request]

    firstelement = content.find_first_of(";");

    artic.ID = atoi(content.substr(0,firstelement).c_str());
    //cout<<"artic.ID "<<artic.ID<<endl;
    content = content.substr(firstelement + 1);//[rep_ID;  request]
   
    content = content.substr(0, content.find_last_of(";"));//delet port
    content = content.substr(0,content.find_last_of(";") - 1);//delet ip

   
    artic.content.push_back(content.substr(content.find_first_of(";") + 1));//add article
    //cout<<"artic.content "<<artic.content[0]<<endl;
    this->articles.push_back(artic);
    return true;
};
 void seq_server::forwardW2(string request)
 {
     // forward request to primary
    string commd = "forward_req;";
    string localip = checkip();
    commd.append(localip).append(";");
    commd.append(to_string(BACKUPPORT)).append(";");//[command;ip;port;request]
    
    commd.append(request);
    //send W2 to primary
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, commd.c_str());
    
 }


void seq_server::Post_handler(const SServerInfo&  client_addr, string request)
{
    string commd = "-1;";//reply ID=-1 for new post
    string localip = checkip();
    request.append(";").append(localip).append(";");
    request.append(to_string(BACKUPPORT));//tell primary how to send back:[request;ip;port]


    this->forwardW2(commd.append(request));//send w2
    //cout<< "commd in back up "<<commd<<endl;
    string ReceivedData; 
    
   
    //receive w5
    ReceivedData = receive(BACKUPPORT);
    //cout<< "w5 in back up "<<ReceivedData<<endl;
    // send ACK_complete to client
    UDP_send(client_addr.m_szIPAddress,client_addr.m_nPort, ReceivedData.c_str());
    //cout<<"Finish post"<<endl;
};
void seq_server::Read_handler(const SServerInfo&  client_addr)
{
    this->sendArticle(client_addr.m_szIPAddress,client_addr.m_nPort);
}; 
void seq_server::Choose_handler(const SServerInfo&  client_addr, string request)
{

}; 
void seq_server::Reply_handler(const SServerInfo&  client_addr, string request)
{
    string commd;//
    string localip = checkip();
    request.append(";").append(localip).append(";");
    request.append(to_string(BACKUPPORT));//tell primary how to send back:[request;ip;port]


    this->forwardW2(commd.append(request));//send w2
    //cout<< "commd in back up "<<commd<<endl;
    string ReceivedData; 
    
    //receive w5
    ReceivedData = receive(BACKUPPORT);
    //cout<< "w5 in back up "<<ReceivedData<<endl;
    // send ACK_complete to client
    UDP_send(client_addr.m_szIPAddress,client_addr.m_nPort, ReceivedData.c_str());
    //cout<<"Finish post"<<endl;
};

void * seq_server:: primary_backup_listenImpl(void * pData)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    vector<int> ack_num(100,0);
    string localip = checkip();
    string strCommand;
    string strIPAddress;
    int nPort = 0;
    COMMAND_TYPE command_type = UNKNOWN_COMMAND;

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

       if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "forward_req") == 0) // only for primary
		{
			command_type = FORWARD;
		}
		else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "ACK_up") == 0) // only for primary
		{
			command_type = ACKUP;
		}
        else if(strcasecmp(strCommand.c_str(), "update") == 0) //Receive w3
		{
			command_type = UPDATE;
		}
        // else if(strcasecmp(strCommand.c_str(), "ACK_comp") == 0)
		// {
		// 	command_type = ACKCOMP;
		// }
		else
		{
			command_type = UNKNOWN_COMMAND;
			cout << "Unknown command" << endl;
		}
        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());

        if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
        {
            //Handle Register Command
            //[command;ip;port;request]
            //cout<< "ReceivedData in primary "<<ReceivedData<<endl;
            for(int i = 0;i<3;++i)
            {
               ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1); 
            }
            string request = ReceivedData;

            if(command_type == FORWARD) // receive w2
            {
                vector<int>::iterator iElementFound = find(ack_num.begin(), ack_num.end(), 0); 
                int req_num;
                if(iElementFound == ack_num.end())
                {
                    req_num = ack_num.size();
                    ack_num.push_back(0);
                    
                }
                else{
                    req_num = distance(ack_num.begin(),iElementFound); //begin from 0
                }
                
                //cout<< "request in primary "<<request<<endl;

                this->forward_update(request,req_num); //send W3
            } 
            else if(command_type == ACKUP)//[req_ID;0/1;origin IP;origin port]
            {
                //cout<<"request in ACKUP "<<request<<endl;
                size_t  firstelement = request.find_first_of(";");
                int req_ID = atoi(request.substr(0,firstelement).c_str());

                //cout<<"req_ID: "<<req_ID<<endl;
                request = request.substr(firstelement + 1);//[0/1;IP;port]
                ack_num[req_ID] ++;
                //cout<<"SIPs.size() "<<SIPs.size()<<endl;
                if(ack_num[req_ID] == SIPs.size())//send W5
                {
                    // send ACK_complete to client
                    string strReturn = "ACK_comp";
                    firstelement = request.find_first_of(";");
                    string succ = request.substr(0,firstelement);//0/1
                    strReturn.append(";").append(succ);//
                    if(atoi(succ.c_str())==0)
                    {
                        --seq_server::ID;//revork ID
                    }
                    request = request.substr(firstelement + 1);//[IP; port]
                    firstelement = request.find_first_of(";");
                    string backupIP = request.substr(0,firstelement);//ip

                    int backupPort =  atoi(request.substr(firstelement + 1).c_str());//port
                    //cout<< "origin ip "<<backupIP<<" origin port "<<backupPort<<endl;
                    UDP_send(backupIP.c_str(),backupPort, strReturn.c_str());
                    //sendto(listenfd, strReturn.c_str(), strReturn.length(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                    ack_num[req_ID] = 0; //return the key
                }
            }
            else if(command_type == UPDATE)
            {
                //receive w3
                //cout<< "w3 in back up "<<request<<endl;
                // update
                bool exist = update_backup(request);
                //send W4 to primary
                string commd = "ACK_up;";
                commd.append(localip).append(";");
                commd.append(to_string(PRIMARYPORT)).append(";");//[command;ip;port]

                string req_ID = request.substr(0,request.find_first_of(";"));//req_ID
                commd.append(req_ID).append(";");//[command;ip;port;req_ID]
                if(exist)
                {
                    cout<< "update_backup success! "<<endl;
                    commd.append("1;");
                }
                else
                {
                    commd.append("0;");
                }
                string port = request.substr(request.find_last_of(";") + 1);//origin port
                string content = request.substr(0, request.find_last_of(";"));//delet port from request
                string ip = content.substr(content.find_last_of(";") + 1);//ip
                commd.append(ip).append(";");
                commd.append(port);//[command;ip;port;req_ID;0/1;origin ip; origin port]
                //cout<< "commd in update_backup "<<commd<<endl;
                UDP_send(strIPAddress.c_str(), PRIMARYPORT, commd.c_str());
            }
            // else if(command_type == ACKCOMP)
            // {
            //     //null
            // }
        }
        else{
            cout << currentDateTime() << " : Invaild IP or Port number" << endl;
        }
   }
   close(listenfd);
   return nullptr;
}
void*  seq_server::primary_backup_listen(void * pData)
{
    mypara *pstru;
    seq_server* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<seq_server *>(pstru -> _this))
    {
        
        pThis->primary_backup_listenImpl((void *)(pstru->Is_live));
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}

/***************************************Local write protocal************************************************
 * *************************************************************************************************
 * ***********************************************************************************************/
void * LocalWrite_server:: LocalWrite_listenImpl(void * pData)
{
    int listenfd = 0;
	int nReceivedBytes = 0;
	socklen_t nClientAddr = 0;
	char szReceivedData[BUFLEN + 1];
	struct sockaddr_in  client_addr;
	bool * Is_live = (bool*) pData;
    vector<int> ack_num(100,0);
    string localip = checkip();
    string strCommand;
    string strIPAddress;
    int nPort = 0;
    COMMAND_TYPE_L command_type = UNKNOWN_COMMAND_L;

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

       if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "moveX") == 0) // only for primary
		{
			command_type = MOVEX;
		}
        else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "forward_req") == 0) 
		{
			command_type = FORWARD_L;
		}
        else if(strcasecmp(strCommand.c_str(), "broadcastIP") == 0)
        {
            command_type = PRIMCHAN;
        }
		else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "ACK_up") == 0) // only for primary
		{
			command_type = ACKUP_L;
		}
        else if(strcasecmp(strCommand.c_str(), "update") == 0) //Receive w3
		{
			command_type = UPDATE_L;
		}
		else
		{
			command_type = UNKNOWN_COMMAND_L;
			cout << "Unknown command" << endl;
		}
        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());

        if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true)
        {
            //Handle Register Command
            //[command;ip;port;request]
            //cout<< "ReceivedData in primary "<<ReceivedData<<endl;
            for(int i = 0;i<3;++i)
            {
               ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1); 
            }
            string request = ReceivedData;

            if(command_type == MOVEX) // send articles to new primary
            {
                this->sendArticle(strIPAddress.c_str(),nPort);

            }
            else if(command_type == PRIMCHAN)
            {
                this->coordinatorIP = strIPAddress;
                this->IsCoordinator = false;
            }
            else if(command_type == FORWARD_L) // receive going on
            {
                vector<int>::iterator iElementFound = find(ack_num.begin(), ack_num.end(), 0); 
                int req_num;
                if(iElementFound == ack_num.end())
                {
                    req_num = ack_num.size();
                    ack_num.push_back(0);
                    
                }
                else{
                    req_num = distance(ack_num.begin(),iElementFound); //begin from 0
                }
                //cout<< "request in primary "<<request<<endl;
                // local backup
                int articleID = this->articles.size() + 1;
                bool localup = update_local(request,articleID);
                // send ACK_complete to client
                string commd = "ACK_comp;";
                if(localup)
                    commd.append("1");
                else
                    commd.append("0");
                UDP_send(strIPAddress.c_str(),nPort, commd.c_str());
                //cout<<"Finish W3"<<endl;
                //send W4
                this->forward_update(request,articleID,req_num); 
            } 
            else if(command_type == ACKUP_L)//[req_ID;0/1]
            {
                //cout<<"request in ACKUP "<<request<<endl;
                size_t  firstelement = request.find_first_of(";");
                int req_ID = atoi(request.substr(0,firstelement).c_str());

                //cout<<"req_ID: "<<req_ID<<endl;
                request = request.substr(firstelement + 1);//[0/1]
                if(atoi(request.c_str())==0)
                {
                    cerr<<"fail in updating!"<<endl;
                }
                else{
                    ack_num[req_ID] ++;
                    if(ack_num[req_ID] == SIPs.size() -1)//send W5
                    {
                        cout<<"success updating!"<<endl;
                    }
                }
                
            }
            else if(command_type == UPDATE_L)
            {
                //receive w4
                //cout<< "w4 in back up "<<request<<endl;
                // update
                bool exist = update_backup(request);
                //send W5 to primary
                string commd = "ACK_up;";
                commd.append(localip).append(";");
                commd.append(to_string(PRIMARYPORT)).append(";");//[command;ip;port]

                string req_ID = request.substr(0,request.find_first_of(";"));//req_ID
                commd.append(req_ID).append(";");//[command;ip;port;req_ID;0/1]
                if(exist)
                {
                    //cout<< "update_backup succ"<<endl;
                    commd.append("1");
                }
                else
                {
                    commd.append("0");
                }
                //cout<< "commd in update_backup "<<commd<<endl;
                UDP_send(strIPAddress.c_str(), PRIMARYPORT, commd.c_str());
            }
        }
        else{
            cout << currentDateTime() << " : Invaild IP or Port number" << endl;
        }
   }
   close(listenfd);
   return nullptr;
}
void*  LocalWrite_server::LocalWrite_listen(void * pData)
{
    mypara *pstru;
    LocalWrite_server* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<LocalWrite_server *>(pstru -> _this))
    {
        
        pThis->LocalWrite_listenImpl((void *)(pstru->Is_live));
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}

LocalWrite_server:: LocalWrite_server(bool Coord):Server(Coord)
{

}
void LocalWrite_server:: Post_handler(const SServerInfo&  client_addr, string request)
{
    string commd = "-1;";//reply ID=-1 for new post
    string localip = checkip();
    request =  commd.append(request);//[-1;request]
    MoveItemX(localip);

    //call itself to go on
    commd = "forward_req;";
    commd.append(client_addr.m_szIPAddress).append(";");
    commd.append(to_string(client_addr.m_nPort)).append(";");//[command;client ip;client port; request]
    commd.append(request);
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, commd.c_str());

}
void LocalWrite_server:: Read_handler(const SServerInfo&  client_addr)
{
    this->sendArticle(client_addr.m_szIPAddress,client_addr.m_nPort);
}
void LocalWrite_server:: Choose_handler(const SServerInfo&  client_addr, string request)
{
}
void LocalWrite_server:: Reply_handler(const SServerInfo&  client_addr, string request)
{
    string localip = checkip();
    MoveItemX(localip);

    //call itself to go on
    string  commd = "forward_req;";
    commd.append(client_addr.m_szIPAddress).append(";");
    commd.append(to_string(client_addr.m_nPort)).append(";");//[command;client ip;client port; request]
    commd.append(request);
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, commd.c_str());

}

 void LocalWrite_server::MoveItemX(const string& localip)//W2,move to current server
 {
    if(this->IsCoordinator)
    {
        return;  // current is the primary
    }
    else
    {
        // forward request to primary
        string commd = "moveX;";
        
        commd.append(localip).append(";");
        commd.append(to_string(BACKUPPORT)).append(";");//[moveX;ip;port]
        //require whole article from original item
        UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, commd.c_str());

        string ReceivedData;
        //receive origin article
        ReceivedData = receive(BACKUPPORT);
        cout<<"recieved origin article from old primary： "<<ReceivedData<<endl; //[content;hiearchy$]
        // store article
        if(ReceivedData!="")
        {
            this->articles.clear();
            CStringTokenizer stringTokenizer;
            stringTokenizer.Split(ReceivedData, "$");
            string article;
            int tmpID = 1;
            while((article =stringTokenizer.GetNext())!="")
            {
                string content = article.substr(0, article.find_last_of(";"));
                int  hierachy = atoi(article.substr(article.find_last_of(";") + 1).c_str());
                Article artic;
                artic.ID = tmpID++;
                artic.content.push_back(content);
                artic.hierachy = hierachy;
                this->articles.push_back(artic);
            }
        }
        // change this to be the primary
        commd = "broadcastIP;";
        commd.append(localip).append(";");
        commd.append(to_string(BACKUPPORT)).append(";");//[command;ip;port]
        vector<SServerInfo>::iterator it;
        this->IsCoordinator = true;
        this->coordinatorIP = localip;
        for(it=SIPs.begin();it!=SIPs.end();it++)//broadcast new primary IP
        {
            if(it->m_szIPAddress == localip) continue;
            UDP_send(it->m_szIPAddress,PRIMARYPORT, commd.c_str()); 
        }
    }
    return;
 }

void LocalWrite_server::forward_update(string request,int articeID, int req_num) //send W4
{
    vector<SServerInfo>::iterator it;
    string localip = checkip();

    requestQ.push(request); // push current back

    string commd = "update;";
    commd.append(localip).append(";");//primary  ip
    commd.append(to_string(PRIMARYPORT)).append(";");//port


    commd.append(std::to_string(req_num)).append(";"); 
 
    commd.append(to_string(articeID)).append(";");//[update;primary ip;port;req_ID;article ID;repID; request]
    string req = requestQ.front();//pop head out
    requestQ.pop();

    commd.append(req);
    //cout<< "commd in forward_update "<<commd<<endl;
    for(it=SIPs.begin();it!=SIPs.end();it++)//broad cast W4
    {
        if(it->m_szIPAddress ==localip) continue;
        UDP_send(it->m_szIPAddress,PRIMARYPORT, commd.c_str()); 
    }
    
    return;
    
}
bool LocalWrite_server::update_local(string request,int articleID)//[rep_ID; request]
{
    //update storage
	CStringTokenizer stringTokenizer;
    Article artic;
    stringTokenizer.Split(request, ";");//[ rep_ID; other request]
    int repID = atoi(stringTokenizer.GetNext().c_str());//ID
    //cout<<"repID "<<repID<<endl;

    if(repID!= -1)
    {
        vector<Article>::iterator it = this->findArticlebyID(repID);
        if(it!=articles.end())
        {
            artic.hierachy = it->hierachy + 1;
        }
        else
        {
            perror("Not found article");
            return false;
        }
    }
    else
    {
        artic.hierachy = 0;
    }
    
    
    artic.ID = articleID;
    //cout<<"artic.ID "<<artic.ID<<endl;
    artic.content.push_back(request.substr(request.find_first_of(";") + 1));//add article
    //cout<<"artic.content "<<artic.content[0]<<endl;
    this->articles.push_back(artic);
    return true;

};
bool LocalWrite_server::update_backup(string request)
{
    
    string localip = checkip();
    Article artic;
    //update storage
	CStringTokenizer stringTokenizer;
    stringTokenizer.Split(request, ";");//[req_ID;article ID; rep_ID; request]
    stringTokenizer.GetNext(); //req_ID
    int contentID = atoi(stringTokenizer.GetNext().c_str());//aritcle ID
    int repID = atoi(stringTokenizer.GetNext().c_str());//ID


    if(repID!= -1)
    {
        vector<Article>::iterator it = this->findArticlebyID(repID);
        if(it!=articles.end())
        {
            artic.hierachy = it->hierachy + 1;
        }
        else
        {
            perror("Not found article");
            return false;
        }
    }
    else
    {
        artic.hierachy = 0;
    }
    
    size_t  firstelement = request.find_first_of(";");//[req ID; content ID; rep_ID;  request]
    string content = request.substr(firstelement + 1);//[content ID; rep_ID;  request]
    firstelement = content.find_first_of(";");

    artic.ID = atoi(content.substr(0,firstelement).c_str());
    content = content.substr(firstelement + 1);//[rep_ID;  request]

    artic.content.push_back(content.substr(content.find_first_of(";") + 1));//add article
    this->articles.push_back(artic);
    return true;

};

/*******************************************************************
 * *******************************quorum consistency*****************************
 * *******************************************************************/

qum_server:: qum_server(bool Coord):Server(Coord)
{
    cout << "Input N, Nr, Nw, Spliteed by ;" << endl;
    char buffer[BUFLEN];
    int N, Nr, Nw;

    bool sign = false;
    while(sign != true){
        cin.getline(buffer, BUFLEN);
        CStringTokenizer bufferTokenizer;
        bufferTokenizer.Split(buffer, ";");
        
        N = stoi(bufferTokenizer.GetNext());
        Nr = stoi(bufferTokenizer.GetNext());
        Nw = stoi(bufferTokenizer.GetNext());
        if(isValidQ(N, Nr, Nw)){
            sign = true;
        } else {
            cout << "Invalid Input, Please Re-input the N, Nr, Nw:" << endl;
        }
        
    }
    this->N = N;
    this->Nr = Nr;
    this->Nw = Nw;

}

vector<SServerInfo> qum_server::selectRandomServer(int size, int select, vector<SServerInfo> IPs){
    
    int quorum[size];
    for(int i = 0; i < size; i++){
        quorum[i] = i;
    }
    for(int i = size-1; i > 0;--i){
        swap(quorum[i], quorum[rand()%i]);
    }

    // list random 
    cout << "Final Order: ";
    for(int i = 0;i < size;i++){
        cout << quorum[i] << " ";
    }
    cout << endl;

    // select num of index
    vector<SServerInfo> serverSelected;
    for(int i = 0;i<select;i++){
        // select the quorum[i] index of server
        vector<SServerInfo>::iterator current = findServerByIndex(quorum[i], IPs);


        // hardcode
        if(quorum[i] == 0){
            current = this->SIPs.begin();
        }
        


        cout << quorum[i] << endl;
        cout << "   IP: " << current->m_szIPAddress << endl;
        cout << "   Port: " << current->m_nPort << endl;

        SServerInfo newServerInfo;
        newServerInfo.m_nPort = current->m_nPort;
        memcpy(newServerInfo.m_szIPAddress, current->m_szIPAddress, sizeof(newServerInfo.m_szIPAddress));
        serverSelected.push_back(newServerInfo);
        cout << "---> Add " << newServerInfo.m_szIPAddress << " to List" << endl;
        
    }
    return serverSelected;
}

vector<SServerInfo>::iterator qum_server::findServerByIndex(int index, vector<SServerInfo> IPs)
{
	vector<SServerInfo>::iterator it = IPs.begin();
    if(index == 0){
        return it;
    }
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

string qum_server::getNewestServer(vector<SServerInfo> serverSelected){
    int selectedServerSize = serverSelected.size();
    vector<SServerInfo>::iterator it = serverSelected.begin();

    vector<SServerInfo>::iterator newest;
    int maxVer = 0;

    string ip = checkip();
    string port = to_string(GETNEWVER_PORT);
    string command = "REQUESTVER;READ;";   // bait

    command.append(ip);
    command.append(";");
    command.append(port);

    
    for(int i = 0; i < selectedServerSize;i++){
        string ReceivedData;
        int currentVer;
        cout << "   Request: " << command << endl;
        cout << "Send Request for version to " << it->m_szIPAddress << endl;
        // hard code
        if(it->m_szIPAddress == checkip()){
            cout << "Send get version request locally" << endl;
            currentVer = this->VER;
        } else {
            UDP_send(it->m_szIPAddress,PRIMARYPORT,command.c_str());
            ReceivedData = receive(GETNEWVER_PORT);
            currentVer = atoi(ReceivedData.c_str());
        }

        cout << "--->Received ver num: " << currentVer << endl;
        
        if(currentVer >= maxVer){
            newest = it;
            cout << "CHANGE MAX ver to :" <<currentVer << endl;
            maxVer = currentVer;
        }
        it++;
    }
    cout << "Newest server is: " << newest->m_szIPAddress << endl;
    return string(newest->m_szIPAddress);

}

bool qum_server::writeSelectedServer(vector<SServerInfo> serverSelected, string content, int preHie){
    int selectedServerSize = serverSelected.size();
    vector<SServerInfo>::iterator it = serverSelected.begin();

    string ip = checkip();
    string port = to_string(WRITESELECT_PORT);
    string command = "OPERATION_WRITE;WRITE;";   // bait
    command.append(ip);
    command.append(";");
    command.append(port);
    command.append(";");
    command.append(to_string(preHie));
    command.append(";");
    command.append(content);

    for(int i = 0; i < selectedServerSize;i++){
        cout << "   Request: " << command << endl;
       
        cout << "Send Write Request to " << it->m_szIPAddress << endl;


        // hardcode
        if(it->m_szIPAddress == checkip()){
            cout << "Locally Write content " << content << endl;
            // local operation
            this->writeArticle(content,preHie);
            it++;
            continue;
        }
        UDP_send(it->m_szIPAddress,PRIMARYPORT,command.c_str());

        string ReceivedData;
        ReceivedData = receive(WRITESELECT_PORT);
        cout << "--->Received ver num: " << ReceivedData << endl;
        int currentVer = atoi(ReceivedData.c_str());
        it++;

    }
    cout << "Finish All Write" << endl;

    return true;

    
}


bool qum_server::loadArticle(string content){

    // same with client load article, but this time load with Article Class
    this->articles.clear();

    CStringTokenizer bufferTokenizer;
    CStringTokenizer articleTokenizer;

     bufferTokenizer.Split(content, "$");
    int bufferLen = bufferTokenizer.GetSize();
    cout << "Total Article Number: " << bufferLen << endl;
    for(int i = 0; i < bufferLen;i++){
        // display the article i+1
        //cout << "loading article " << i+1 << endl;
        Article newArticle;

        string article = bufferTokenizer.GetNext();  //cout << "   new article: " << article << endl;
        articleTokenizer.Split(article, ";");

        // fit content

        newArticle.ID = i+1;

        string contentBuf;
        //contentBuf.append(";");

        for(int j = 0; j < 4;j++){
            contentBuf.append(articleTokenizer.GetNext());
            contentBuf.append(";");
        }
        contentBuf.append(articleTokenizer.GetNext());

        cout << "Final content Buffer: " << contentBuf << endl;
        newArticle.content.push_back(contentBuf);


        string hierachy = articleTokenizer.GetNext();
        newArticle.hierachy = stoi(hierachy); //cout << "   hierachy: " << hierachy << endl;

        (this->articles).push_back(newArticle);

    }

}



bool qum_server::synchSelectedServer(vector<SServerInfo> serverSelected, string content){
    int selectedServerSize = serverSelected.size();
    vector<SServerInfo>::iterator it = serverSelected.begin();

    string ip = checkip();
    string port = to_string(SYNCH_PORT);
    string command = "OPERATION_SYNCH;SYNCH;";   // bait
    command.append(ip);
    command.append(";");
    command.append(port);
    command.append(";");
    command.append(content);

    for(int i = 0; i < selectedServerSize;i++){
        cout << "   Request: " << command << endl;
       
        cout << "Send Synch Request to " << it->m_szIPAddress << endl;


        // hardcode
        if(it->m_szIPAddress == checkip()){
            cout << "Locally Synch content " << content << endl;
            // local operation
            loadArticle(content);
            it++;
            continue;
        }
        UDP_send(it->m_szIPAddress,PRIMARYPORT,command.c_str());

        string ReceivedData;
        ReceivedData = receive(SYNCH_PORT); //finish write
        cout << "--->Received ver num: " << ReceivedData << endl;
        it++;

    }
    cout << "Finish All Synch" << endl;

    return true;

    
}


void qum_server::writeArticle(string content){
    CStringTokenizer articleTokenizer;
    articleTokenizer.Split(content, ";");

    Article newArticle;
    this->ID += 1;
    newArticle.ID = this->ID;
    newArticle.content.push_back(content);
    newArticle.hierachy = 0;
    newArticle.primaryIP = checkip();

    this->articles.push_back(newArticle);

    this->VER += 1;

}

void qum_server::writeArticle(string content, int preHierarchy){
    CStringTokenizer articleTokenizer;
    articleTokenizer.Split(content, ";");

    cout << content << endl;

    Article newArticle;
    this->ID += 1;
    newArticle.ID = this->ID;
    newArticle.content.push_back(content);
    newArticle.hierachy = preHierarchy + 1;
    newArticle.primaryIP = checkip();

    articles.push_back(newArticle);

    this->VER += 1;

}



string qum_server::generatePrefix(const SServerInfo client_addr, string request, string TYPE, int preHie){
    TYPE.append(";");
    TYPE.append(client_addr.m_szIPAddress);
    TYPE.append(";");
    TYPE.append(to_string(client_addr.m_nPort));
    TYPE.append(";");
    if(preHie != -2){
        TYPE.append(to_string(preHie));
        TYPE.append(";");
    }

    TYPE.append(request);
    return TYPE;
}




void qum_server::Post_handler(const SServerInfo&  client_addr, string request){
    // [POST;host;port;content]
    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, request, "POST", -1);
    requestCont.append(requestLast);
    cout << "POST: Request Content: " << requestCont << endl;

    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}

void qum_server::Read_handler(const SServerInfo&  client_addr){
     // [POST;host;port;content]
    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, "", "READ",-2);
    requestCont.append(requestLast);
    cout << "READ: Request Content: " << requestCont << endl;


    // extra test

    cout << "Test SIPs" << endl;
    cout << SIPs.begin()->m_szIPAddress << endl;
    cout << SIPs.begin()->m_nPort << endl;
    cout << "Article SIze: " << this->articles.size() << endl; 
   
    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}
void qum_server::Choose_handler(const SServerInfo&  client_addr, string request){

    // this is actually for synch...

    string requestCont = "FORWARD_QUM;";
    string requestLast = generatePrefix(client_addr, "", "SYNCH",-2);
    requestCont.append(requestLast);
    cout << "SYNCH: Request Content: " << requestCont << endl;

    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());


} 
void qum_server::Reply_handler(const SServerInfo&  client_addr, string request){
    

    CStringTokenizer replyTokenizer;
    replyTokenizer.Split(request,";");
    int id = stoi(replyTokenizer.GetNext());
    cout << "Previous ID: "<< id << endl;
    int preHie;


    string newestHost = getNewestServer(SIPs);
    if(newestHost == checkip()){
        vector<Article>::iterator it = findArticlebyID(id);
        preHie = it->hierachy;
        cout << "Previous Hie: " << preHie << endl;
    } else {
        string command_hie = "OPERATION_REPLY;REPLY;";
        command_hie.append(checkip());
        command_hie.append(";");
        command_hie.append(to_string(REPLY_PORT));
        command_hie.append(";");
        command_hie.append(to_string(id));
        
        UDP_send(newestHost.c_str(),PRIMARYPORT, command_hie.c_str());

        string ReceivedData;
        ReceivedData = receive(REPLY_PORT);

        preHie = stoi(ReceivedData);
        cout << "Previous Hie: " << preHie << endl;
    }


    string requestCont = "FORWARD_QUM;";
    request = request.substr(request.find_first_of(";") + 1); 
    string requestLast = generatePrefix(client_addr, request, "POST", preHie);
    requestCont.append(requestLast);
    cout << "REPLY: Request Content: " << requestCont << endl;

   
    // forward
    UDP_send(this->coordinatorIP.c_str(), PRIMARYPORT, requestCont.c_str());

}

void qum_server::FORWARD_handler(string request){
    // situation: primary receive forward from a server
    // request: [POST/READ/REPLY; clienthost;clientport]

    cout << "Request At FORWARD hendler: " << request << endl;
    CStringTokenizer requestTokenizer;
    requestTokenizer.Split(request, ";");
    string command = requestTokenizer.GetNext();

    if (strcasecmp(command.c_str(), "READ") == 0){
        // read quorum
        cout << "   Pick Random Serverlist..." << endl;
        vector<SServerInfo> serverList = selectRandomServer(this->N, this->Nr, this->SIPs);

        cout << "   Pick Newest Version Info..." << endl;
        string newest = getNewestServer(serverList);

        string clienthost = requestTokenizer.GetNext();
        string clientport = requestTokenizer.GetNext();


        // get article content
        string command = "OPERATION_READ;READ;";
        command.append(clienthost);
        command.append(";");
        command.append(clientport);

        cout << "Ask Server ";
        cout << newest;
        cout << "Send articles to client" << endl;
        cout << "---> Command: " << command << endl;


        // hardcode
        if(newest == checkip()){
            cout << "Locally Send newest to client " << endl;
            this->sendArticle(clienthost.c_str(), stoi(clientport));
            return;

        }

        UDP_send(newest.c_str(), PRIMARYPORT, command.c_str());



    } else if (strcasecmp(command.c_str(), "POST") == 0){
        // write quorum
        cout << "   Pick Random Serverlist..." << endl;
        vector<SServerInfo> serverList = selectRandomServer(this->N, this->Nr, SIPs);

        // write all quorum

        // wash request
        for(int i = 0;i<4;++i)
        {
            request = request.substr(request.find_first_of(";") + 1); 
        }
        cout << "WRITE: Washed request: " << request << endl;

        // send feedback
        string clienthost = requestTokenizer.GetNext();
        string clientport = requestTokenizer.GetNext();
        string preHie_s = requestTokenizer.GetNext();
        int preHie = stoi(preHie_s);

        bool result = writeSelectedServer(serverList, request, preHie);


        // synch selected group

        string newestHost = getNewestServer(serverList);
        if(newestHost == checkip()){
            // pack article to self
            sendArticle(checkip().c_str(), REQUESTPORT);


        } else { // read newest
            string command = "OPERATION_READ;SYNCH;";
            command.append(checkip());
            command.append(";");
            command.append(to_string(REQUESTPORT));
            
            UDP_send(newestHost.c_str(),PRIMARYPORT, command.c_str());

            // broadcast this article to everyo
        }
        string ReceivedData;  // receive lastest article
        ReceivedData = receive(REQUESTPORT);

        if(ReceivedData != ""){
            // update all
            synchSelectedServer(SIPs,ReceivedData);
        }





        
        if(result == true){
            UDP_send(clienthost.c_str(),stoi(clientport),"Success;1");
        } else {
            UDP_send(clienthost.c_str(),stoi(clientport),"Fail;0");
        }


    } else if (strcasecmp(command.c_str(), "SYNCH") == 0){



        // find newest
        string newestHost = getNewestServer(SIPs);
        if(newestHost == checkip()){
            // pack article to self
            sendArticle(checkip().c_str(), REQUESTPORT);


        } else { // read newest
            string command = "OPERATION_READ;SYNCH;";
            command.append(checkip());
            command.append(";");
            command.append(to_string(REQUESTPORT));
            
            UDP_send(newestHost.c_str(),PRIMARYPORT, command.c_str());

            // broadcast this article to everyo
        }
        string ReceivedData;  // receive lastest article
        ReceivedData = receive(REQUESTPORT);

        if(ReceivedData != ""){
            // update all
            synchSelectedServer(SIPs,ReceivedData);
        }

       

        //send feedback
        string clienthost = requestTokenizer.GetNext();
        string clientport = requestTokenizer.GetNext();
        UDP_send(clienthost.c_str(),stoi(clientport),"Success;1");



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
            cout << "Get Command FORWARD" << endl;
		}
		else if(this->IsCoordinator && strcasecmp(strCommand.c_str(), "RECEIVEVER") == 0) // only for primary
		{
			command_type = RECEIVEVER;
            cout << "Get Command RECEIVEVER" << endl;
		}
        else if(strcasecmp(strCommand.c_str(), "REQUESTVER") == 0) 
		{
			command_type = REQUESTVER;
		}
         else if(strcasecmp(strCommand.c_str(), "REQUESTART") == 0) 
		{
			command_type = REQUESTART;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_READ") == 0)
		{
			command_type = OPERATION_READ;
		}
        else if(strcasecmp(strCommand.c_str(), "OPERATION_WRITE") == 0)
		{
			command_type = OPERATION_WRITE;
            cout << "Get Command OPERATION_WRITE" << endl;
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

        string operation = stringTokenizer.GetNext(); // READ / WRITE/ REPLy
        strIPAddress = stringTokenizer.GetNext();
        nPort = atoi(stringTokenizer.GetNext().c_str());

        cout << "   request IP: " << strIPAddress << endl;
        cout << "   request Port: " << nPort << endl;

        if(isValidIP(strIPAddress) == true && isValidPort(nPort) == true){
            /*
            cout<< "ReceivedData in listen: "<<ReceivedData<<endl;
            for(int i = 0;i<3;++i)
            {
               ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1); 
            }
            */
            ReceivedData = ReceivedData.substr(ReceivedData.find_first_of(";") + 1);
            string request = ReceivedData;
            cout << "---> request: " << request << endl;

            // REceivedData -> [host;port;content]

            if(command_type == FORWARD_QUM){ // only primary

                // deal with received forward message
                FORWARD_handler(request);

            } else if (command_type == RECEIVEVER){ // only primary
                // ditto
            } else if (command_type == REQUESTART){ // only primary
                // send current article to this server
                this->sendArticle(strIPAddress.c_str(), nPort);

            } else if (command_type == REQUESTVER){
                // get version number and send back
                UDP_send(strIPAddress.c_str(), nPort, (to_string(this->VER).c_str()));
            } else if (command_type == OPERATION_READ){

                // send article to the target server with designated port and host
                cout << "OP_READ: Send article to client" << endl;
                this->sendArticle(strIPAddress.c_str(), nPort);

            } else if (command_type == OPERATION_WRITE){

                // write
                cout << "OP_WRITE: Write article at local" << endl;
                for(int i = 0;i<4;++i)
                {
                    request = request.substr(request.find_first_of(";") + 1); 
                }

                int preHie = stoi(stringTokenizer.GetNext());


                writeArticle(request,preHie);
                UDP_send(strIPAddress.c_str(), nPort, (to_string(this->VER).c_str()));
                
            } else if (command_type == OPERATION_REPLY){

                // send article to the target server with designated port and host
                cout << "OP_REPLY: Send preHie to server" << endl;
                int id = stoi(stringTokenizer.GetNext());
                string preHie = to_string((this->findArticlebyID(id))->hierachy);
                cout << "   preHie:  "<<preHie << endl;
                UDP_send(strIPAddress.c_str(), nPort, preHie.c_str());

            } else if (command_type == OPERATION_SYNCH){

                // send article to the target server with designated port and host

                for(int i = 0;i<3;++i)
                {
                    request = request.substr(request.find_first_of(";") + 1); 
                }
                cout << "SYNCH: load request-> " << request << endl;
                this->loadArticle(request);
                UDP_send(strIPAddress.c_str(), nPort, to_string((this->VER)).c_str());

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

void *qum_server::heartBeatImpl(void * pData)
{
    /*
    struct sockaddr_in si_me, si_other;
	int s,  recv_len;
	socklen_t slen= sizeof(si_other);
	char buf[BUFLEN];
	bool * Is_live = (bool*) pData;
	//create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}
	
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(HBPORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		die("bind");
	}
	//keep listening for data
	//wait list 10 sec.
	timeval tv;
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	while(1)
	{
		fflush(stdout);
		//Set Timeout for recv call
		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(timeval));
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			if(*Is_live)
			{
				continue;
			}
			else
			{
				die("recvfrom()");
			}
		}
		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n" , buf);	
		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
		{
			die("sendto()");
		}
	}
	close(s);
	pthread_exit(NULL);
    */
	int sockfd = 0;
	char szReceivedData[32];
    struct sockaddr_in serv_addr;
	const string strHeartBeat = "heartbeat";
	string strIPAddress;
	int nPort = 0;

    int recvLen, servLen;
    struct hostent *pHostent = NULL;
 


	while(1)
	{
        sleep(4);
        if(this->IsCoordinator == true){
            SServerInfo currentHostInfo;
            memcpy(currentHostInfo.m_szIPAddress, checkip().c_str(), sizeof(currentHostInfo.m_szIPAddress));
            currentHostInfo.m_nPort = HEARTPORT;
            cout << "--> Start Auto Synch " << endl;
            

            string newestHost = getNewestServer(this->SIPs);
            if(newestHost == checkip()){
                // pack article to self
                sendArticle(checkip().c_str(), HEARTBEAT_PORT);


            } else { // read newest
                string command = "OPERATION_READ;SYNCH;";
                command.append(checkip());
                command.append(";");
                command.append(to_string(HEARTBEAT_PORT));
                
                UDP_send(newestHost.c_str(),PRIMARYPORT, command.c_str());

                // broadcast this article to everyo
            }
            string ReceivedData;  // receive lastest article
            ReceivedData = receive(HEARTBEAT_PORT);

            // update all
            synchSelectedServer(SIPs,ReceivedData);
            cout << "--> Finish Auto Synch" << endl;

        } else {
            cout << "--> Pass Auto Synch" << endl;
        }
            

        //string receivedData;
        //receivedData = receive(HEARTPORT);
        
		//Wait 5sec
		sleep(4);
	}






}

void*  qum_server::quorum_heartbeat(void * pData)
{
    mypara *pstru;
    qum_server* pThis;
    pstru = (struct mypara *) pData;
    if(pThis = dynamic_cast<qum_server *>(pstru -> _this))
    {
        
        pThis->heartBeatImpl((void *)(pstru->Is_live));
    }
    else
    {
        perror("dynamic_cast failed");
    }
    pthread_exit(NULL);
}












int main(int argc, char *argv[])
{
    //pthread_t unicast_thread;
    srand( (unsigned)time( NULL ) );
    bool Is_live= true;
	pthread_t LitsenThread;
	pthread_t command_thread;
    pthread_t  primary_backup_listen_thread;
    pthread_t heartBeatThread;
    Server* group;
    struct mypara pstru;
    if (argc !=1 && argc != 2) {
     fprintf(stderr, "usage: %s host message\n", argv[0]);
     exit(-1);
    }
    int type = 0;
    cout<<"please choose the protocal for consistency"<<endl;
    cout<< "1 for sequential consistency; 2 for quorum consistency; 3 for Read-your-Write"<<endl;
    cin>>type;cin.get();
    switch(type)
    {
        case 1: 
           {
               if(argc == 1)
                {
                    group = new seq_server(true); // for coordinator
                }
                else
                {
                    group = new seq_server(false); // orther group
                    char* serverIP = argv[1];
                    int suc_re = group->RegisterS(string(serverIP));
                    if(suc_re<0)
                    {
                        cout << "fail registering to coordinator." << endl;
                        return -1;
                    }
                    
                }
                
                cout << "Group-server starts." << endl;
                //Listen 
                pstru._this = group;
                pstru.Is_live = &Is_live;
                pthread_create(&LitsenThread, NULL, seq_server::listenThread, (void*)&pstru);
                pthread_create(&command_thread, NULL, seq_server::CommandThread, (void*)&pstru);
                pthread_create(&primary_backup_listen_thread, NULL, seq_server::primary_backup_listen, (void*)&pstru); 
            

                //Listen to client
                int nRes = pthread_join(LitsenThread, NULL); 
                if(nRes!=0)
                {
                    perror ("LitsenThread error!");
                }
                int nRes2 = pthread_join(command_thread, NULL);
                if(nRes2!=0)
                {
                    perror ("command_thread error!");
                }
                int nRes3 = pthread_join(primary_backup_listen_thread, NULL);
                if(nRes3!=0)
                {
                    perror ("primary_backup_listen_thread error!");
                }   
                if(nRes||nRes2||nRes3)
                    exit(-1);
                else
                    exit (1);
           }
        break;
        case 2:
         {
               if(argc == 1)
                {
                    group = new qum_server(true); // for coordinator
                    
                }
                else
                {
                    group = new qum_server(false); // orther group
                    char* serverIP = argv[1];
                    int suc_re = group->RegisterS(string(serverIP));
                    if(suc_re<0)
                    {
                        cout << "fail registering to coordinator." << endl;
                        return -1;
                    }
                    
                }
                
                cout << "Group-server starts." << endl;
                //Listen 
                pstru._this = group;
                pstru.Is_live = &Is_live;
                pthread_create(&LitsenThread, NULL, qum_server::listenThread, (void*)&pstru);
                pthread_create(&heartBeatThread, NULL, qum_server::quorum_heartbeat, (void*)&pstru);
                pthread_create(&command_thread, NULL, qum_server::CommandThread, (void*)&pstru);
                pthread_create(&primary_backup_listen_thread, NULL, qum_server::quorum_listen, (void*)&pstru); 
            

                //Listen to client
                int nRes = pthread_join(LitsenThread, NULL); 
                if(nRes!=0)
                {
                    perror ("LitsenThread error!");
                }
                int nRes2 = pthread_join(command_thread, NULL);
                if(nRes2!=0)
                {
                    perror ("command_thread error!");
                }
                int nRes3 = pthread_join(primary_backup_listen_thread, NULL);
                if(nRes3!=0)
                {
                    perror ("primary_backup_listen_thread error!");
                }   
                int nRes4 = pthread_join(heartBeatThread, NULL);
                if(nRes3!=0)
                {
                    perror ("heartBeat_thread error!");
                } 
                if(nRes||nRes2||nRes3||nRes4)
                    exit(-1);
                else
                    exit (1);
           }
        break;
        default:
        {
            if(argc == 1)
                {
                    group = new LocalWrite_server(true); // for coordinator
                }
                else
                {
                    group = new LocalWrite_server(false); // orther group
                    char* serverIP = argv[1];
                    int suc_re = group->RegisterS(string(serverIP));
                    if(suc_re<0)
                    {
                        cout << "fail registering to coordinator." << endl;
                        return -1;
                    }
                    
                }
                
                cout << "Group-server starts." << endl;
                //Listen 
                pstru._this = group;
                pstru.Is_live = &Is_live;
                pthread_create(&LitsenThread, NULL, LocalWrite_server::listenThread, (void*)&pstru);
                pthread_create(&command_thread, NULL, LocalWrite_server::CommandThread, (void*)&pstru);
                pthread_create(&primary_backup_listen_thread, NULL, LocalWrite_server::LocalWrite_listen, (void*)&pstru); 
            

                //Listen to client
                int nRes = pthread_join(LitsenThread, NULL); 
                if(nRes!=0)
                {
                    perror ("LitsenThread error!");
                }
                int nRes2 = pthread_join(command_thread, NULL);
                if(nRes2!=0)
                {
                    perror ("command_thread error!");
                }
                int nRes3 = pthread_join(primary_backup_listen_thread, NULL);
                if(nRes3!=0)
                {
                    perror ("LocalWrite_listen error!");
                }   
                if(nRes||nRes2||nRes3)
                    exit(-1);
                else
                    exit (1);
        }
        break;
    }
}
