# CSCi5105
Group Project for CSCi5105: Distributed System


## Project 1 PubSub System


## Target
Get familiar with RPC and UDP protocal

![System Design](https://github.com/hswsp/Intro_Distributed_System/blob/master/Project1/pubsub.jpg?raw=true)

## Project 2 Bulletin Board Consistency

We acheived 3 consisteny for this distributed system.

![System Design](https://github.com/hswsp/Intro_Distributed_System/blob/master/Project2/system.jpg?raw=true)

## sequential consistency

This means that all clients should see the same order of articles on a read from any server even if they were
posted by concurrent clients to any servers. We use the primary-backup protocol

## quorum consistency

we use the coordinator as a control point for your quorum. That is, the client contacts any server, which
in turn, contacts the coordinator to do the operation contacting the other randomly chosen servers needed for
the quorum. Now, vary the values of NR and NW and measure the cost (as seen from the client) to do a write
or read operation. 

## Read-your-Write consistency

For this, suppose a client C posts an article or reply to a specific server S1. Later, if the client C connects
to a different server S2 and does a read or choose, they are guaranteed to see the prior updates.
We use the local-write protocol.

## Project 3 Simple xFS

In this project, we implemented a simple “serverless” file system based on the classic xFS paper[1] in which
peer nodes can share their files directly with other peers. 

Using this system, a user can store files in their local storage (a specific directory up to you, e.g. ~you/5105/share/machID) to share with other users. The
machID is a unique name that refers to a unique peer. At any point in time, a file may be stored at a number
of peers. When a node wishes to download a file, it learns about the possible locations of the file (from the
server) and makes a decision based on the latency and load (to be described). 

This system will also support basic fault tolerance: any nodes or the server can go down and then recover. 

We used communication protocol UDP. 

![System Design](https://github.com/hswsp/Intro_Distributed_System/blob/master/Project3/system.jpg?raw=true)

## Resource
[1] Serverless Network File System: http://www.cs.iastate.edu/~cs652/notes/xfs.pdf

