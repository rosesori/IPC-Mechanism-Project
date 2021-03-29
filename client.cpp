/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19
 */

#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
#include <chrono>
#include <string>
using namespace std::chrono;
using namespace std;


int main(int argc, char *argv[]){
    int c;                         // how many channels
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;            // patient, ecg #
    double t = -1.0;               // time
    bool isnewchan = false;        // more than 1 channel needed
    bool isfiletransfer = false;
    string filename;
    string ipcmethod = "f";
    int nchannels = 1;             
    
    // Getting command line arguments
    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                break;
            case 'c':
                isnewchan = true;
                nchannels = atoi(optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'i':
                ipcmethod = optarg;
                break;
        }
    }

    // Fork part
    if (fork()==0){ // CHILD** (SERVER)
		char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), "-i", (char*) ipcmethod.c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }

    // Joining the original "control" RequestChannel
    RequestChannel* control_chan = NULL;
    if (ipcmethod == "f") {
        control_chan = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);
    }
    else if (ipcmethod == "q") {
        control_chan = new MQRequestChannel("control", MQRequestChannel::CLIENT_SIDE);
    }
    else if (ipcmethod == "m") {
        control_chan = new SHMRequestChannel("control", SHMRequestChannel::CLIENT_SIDE, buffercap);;
    }
    
    // ----------------------Start Timer ----------------------------
    auto start = high_resolution_clock::now();

    // Implementing multiple channels
    for (int currChannel=1; currChannel<=nchannels; currChannel++) {
        RequestChannel* chan = control_chan;
        if (isnewchan){ 
            cout << "Using the new channel everything following" << endl;
            MESSAGE_TYPE m = NEWCHANNEL_MSG;
            control_chan->cwrite (&m, sizeof (m));
            char newchanname [100];
            control_chan->cread (newchanname, sizeof (newchanname));
            if (ipcmethod == "f") {
                chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if (ipcmethod == "q") {
                chan = new MQRequestChannel(newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if (ipcmethod == "m") {
                chan = new SHMRequestChannel(newchanname, RequestChannel::CLIENT_SIDE, buffercap);;
            }
            cout << "New channel by the name " << newchanname << " is created" << endl;
            cout << "All further communication will happen through it instead of the main channel" << endl;
        }
        if (!isfiletransfer){   // Requesting data msgs
            if (t >= 0){    // 1 data point
                datamsg d (p, t, ecg);
                chan->cwrite (&d, sizeof (d));
                double ecgvalue;
                chan->cread (&ecgvalue, sizeof (double));
                cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
            }else{          // bulk (i.e., 1K) data requests 
                double ts = 0;  
                datamsg d (p, ts, ecg);
                double ecgvalue;
                for (int i=0; i<1000; i++){
                    chan->cwrite (&d, sizeof (d));
                    chan->cread (&ecgvalue, sizeof (double));
                    d.seconds += 0.004; //increment the timestamp by 4ms
                    //cout << ecgvalue << endl; Dont want to print them all anymore
                }
            }
        }
        else if (isfiletransfer){
            // part 2 requesting a file
            filemsg f (0,0);  // special first message to get file size
            int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
            char* buf = new char [to_alloc];
            memcpy (buf, &f, sizeof(filemsg));
            strcpy (buf + sizeof (filemsg), filename.c_str());
            chan->cwrite (buf, to_alloc);
            __int64_t filesize;
            chan->cread (&filesize, sizeof (__int64_t));
            cout << "File size: " << filesize << endl;

            // collect "=filesize/nchannels" bytes from each channel **** I CHANGED HERE
            __int64_t bytesToBeCollected = filesize/nchannels;
            cout << "Bytes to be collected by channel " << currChannel << ": " << bytesToBeCollected << endl;

            // int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
            filemsg* fm = (filemsg*) buf;
            __int64_t rem = bytesToBeCollected; // **** I CHANGED HERE
            string outfilepath = string("received/") + filename;
            FILE* outfile = fopen (outfilepath.c_str(), "wb");  
            fm->offset = 0;

            char* recv_buffer = new char [MAX_MESSAGE];
            while (rem>0){
                fm->length = (int) min (rem, (__int64_t) MAX_MESSAGE);
                chan->cwrite (buf, to_alloc);
                chan->cread (recv_buffer, MAX_MESSAGE);
                fwrite (recv_buffer, 1, fm->length, outfile);
                rem -= fm->length;
                fm->offset += fm->length;
                //cout << fm->offset << endl;
            }
            fclose (outfile);
            delete recv_buffer;
            delete buf;
            cout << "File transfer completed" << endl;
        }

        // Clean up current Channel
        delete chan;
        cout << endl;
    }

    // ----------------------End Timer ----------------------------
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds> (stop-start);
    cout << "Time taken to complete = " << duration.count() << " microseconds, or " << duration.count()/1000000.000 << " seconds." << endl;

    // Clean
    MESSAGE_TYPE q = QUIT_MSG;
    control_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
        
	/* Wait for the child process running server
     * this will allow the server to properly do clean up
     * if wait is not used, the server may sometimes crash */
	wait (0);
}
