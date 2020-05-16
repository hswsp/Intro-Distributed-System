## Project 3 Simple xFS

In this project, we implemented a simple “serverless” file system based on the classic xFS paper[1](http://www.cs.iastate.edu/~cs652/notes/xfs.pdf) in which
peer nodes can share their files directly with other peers. 

Using this system, a user can store files in their local storage (a specific directory up to you, e.g. ~you/5105/share/machID) to share with other users. The
machID is a unique name that refers to a unique peer. At any point in time, a file may be stored at a number
of peers. When a node wishes to download a file, it learns about the possible locations of the file (from the
server) and makes a decision based on the latency and load (to be described). 

This system will also support basic fault tolerance: any nodes or the server can go down and then recover. 

We used communication protocol UDP. 

![System Design](https://github.com/hswsp/Intro_Distributed_System/blob/master/Project3/system.jpg?raw=true)