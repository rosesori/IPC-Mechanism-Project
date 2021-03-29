#include "common.h"
#include "SHMreqchannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int _len) : RequestChannel(_name, _side){
	s1 = "/SHM_" + my_name + "1";
	s2 = "/SHM_" + my_name + "2";
	len = _len;

	shmq1 = new SHMQ (s1, len);
	shmq2 = new SHMQ (s2, len);
	
	if (my_side == CLIENT_SIDE) {
		swap (shmq1, shmq2); // safe to do because we have already opened the shared memory message queue
	}

}

SHMRequestChannel::~SHMRequestChannel(){ 
	delete shmq1; // destorying the two shared message queues (gets rid of semaphores and shared mem segment)
	delete shmq2;
}

int SHMRequestChannel::cread(void* msgbuf, int bufcapacity){
	return shmq1->my_shm_recv(msgbuf, bufcapacity);  // 8 Kilobyte to fool SHM system
}

int SHMRequestChannel::cwrite(void* msgbuf, int len){
	return shmq2->my_shm_send(msgbuf, len);
}

