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

