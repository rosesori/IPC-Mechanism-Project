
#ifndef _ReqChannel_H_
#define _ReqChannel_H_

#include "common.h"

class RequestChannel
{
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};
	enum Mode {READ_MODE, WRITE_MODE};
	
protected:
	/*  The current implementation uses named pipes. */
	string my_name;
	Side my_side;
	
	int wfd;
	int rfd;
	
	string s1, s2;
	virtual int open_ipc(string _pipe_name, int mode){;}
	
public:
	RequestChannel(const string _name, const Side _side): my_name(_name), my_side(_side){}
	/* Creates a "local copy" of the channel specified by the given name. 
	 If the channel does not exist, the associated IPC mechanisms are 
	 created. If the channel exists already, this object is associated with the channel.
	 The channel has two ends, which are conveniently called "SERVER_SIDE" and "CLIENT_SIDE".
	 If two processes connect through a channel, one has to connect on the server side 
	 and the other on the client side. Otherwise the results are unpredictable.

	 NOTE: If the creation of the request channel fails (typically happens when too many
	 request channels are being created) and error message is displayed, and the program
	 unceremoniously exits.

	 NOTE: It is easy to open too many request channels in parallel. Most systems
	 limit the number of open files per process.
	*/

	virtual ~RequestChannel(){}

	virtual int cread(void* msgbuf, int bufcapacity) = 0;
	
	virtual int cwrite(void *msgbuf , int msglen) = 0;
	 
	string name(){
		return my_name;
	}
};

#endif
