/*
    process.h - class Process and class Message
*/

#ifndef  PROCESS_H
#define  PROCESS_H


#include <set>
#include <vector>
#include <queue>    // std::queue
#include <numeric>  // for accumulate()

#include "clock.h"
#include "garg.h"

using namespace std;


//#define _DEBUG_INTERVAL // for debugging interval implementation

/*
  External global variables
*/

extern int number_of_processes;
extern double alpha;
extern distance_matrix D;
extern double localPredRate;
extern Token globalToken;

extern int log2_max_interval;
extern double geometric_dist_param;

extern vector<ofstream> debug_candidate;

extern int base_interval_length;

// just for debug
extern vector<int> candidateIntervalSizeList;
extern vector<int> candidateReadyIntervalSizeList;


/*
  message - structure of a message communicated between processes
*/

struct message{
	message(int arrival_time, const HVC& sender_at, int sender_id) : arrival_time(arrival_time), sender_at(sender_at), sender_id(sender_id){}

	int arrival_time;
	HVC sender_at;
	int sender_id;

  // define the operator <() which is used to order messages in mailbox (i.e. set<message>)
	bool operator < (const message& rhs) const {
		if (arrival_time != rhs.arrival_time) return arrival_time < rhs.arrival_time;
		return sender_id < rhs.sender_id;
	}
};


/*
  Process - class for a process/node in a distributed system
*/

class Process {
private:
	bool time_locked = 0;   // to control RefreshClock() from updating multiple 
	                        // times within one physical clock cycle. 
	                        // When time_locked = false, RefreshClock() will update augmented_time
	                        // when time_locked = true, RefreshClock() does nothing
	                        
	int my_time_now = epsilon;  // time of process at current physical clock cycle
	
	int id =-1;             // process' ID.
	set<message> mail_box;  // collection of messages a process received, ordered by time of reception
	HVC augmented_time;     // hybrid vector clock
	vector<int> size_of_at_history;
	
	// for Garg's algorithm and its interval variation
    std::queue<Candidate> candidateQueue;   // queue of candidates which are ready to be proposed in Garg's algorithm
    std::queue<Candidate> candidateBuffer;  // queue of candidates whose end time has not expired, i.e. still >= my_time_now
                                            // thus it is still possible to be split by some communication event
                                            // candidate generated --deposited--> candidateBuffer --expire--> candidateQueue --proposed--> Garg algo
                                            //                                           ^
                                            //                                           | update candidates in buffer
                                            //                                      send/receive
    
    int candCount = 0;      // to keep track of ID of candidates for each processes
    bool firstflag = true;  // firstflag:   indicator of whether there is an active candidate or not
                            //              if there is, you should not generate new one, otherwise candidates' interval would overlap
                            //              however, there is a special case when there is communication (send/receive). In this case
                            //              you need to generate a new one. To avoid overlapping, you split the old candidate into 2 candidate.
                            //              the first would expire immediately. The second would go into the candidate buffer
                            //          =   true if there is no active candidate or
                            //                      there is communication event
                            //          =   false if there is active candidate
                            //  In Garg's algorithm (which is point-based): firstflag means whether there has been a changed in local vector clock
                            //              Thus, if local pred = true, but firstflag = false, then the state of process has not been changed
                            //                        and we do not need to send candidate since a candidate with this state and vector clock has
                            //                        been sent before
                            //                    if local pred = true, and firstflag = true, this is a new state with new timestamp. We need to
                            //                        send a new candidate to the monitor. After sending, we mark firstflag as fault to avoid
                            //                        sending a duplicate candidate.
                            //  state diagram of firstflag:
                            //      init = true  --- deposit new candidate into candidateBuffer --> false
                            //               ^                                                       |
                            //               +--send/receive/interval expires -----------------------+

    // private methods
	void CountActiveSize();
	int GetTimeNow() const ;
	void RefreshClock();


public:
  // Constructor
	Process(){ }

	Process(int id) : id(id){
    // initialize augmented_time as an HVC with all elements have value -1*epsilon - 1
    // and active_size = 1
    augmented_time = HVC(id, vector<int>(number_of_processes, -1*epsilon-1),1);

    my_time_now = epsilon;
  }
  
  // gets and sets

	int GetId() { return id; }

  // methods for message communication
	void SendMsg(Process& k)  ;
	void ReceiveMsg();
	void BroadcastMsg(vector<Process>& vp);
	void PushMsg(const message& m);
	

  // http://duramecho.com/ComputerInformation/WhyHowCppConst.html
  // const to avoid modifying class members
	double GetSumSizeActive() const{
		return accumulate(begin(size_of_at_history), end(size_of_at_history), 0);
	}

	double GetAVGSizeActive() const{
		return GetSumSizeActive() / (int)size_of_at_history.size();
	}

	int GetNumEvents() const {
		return size_of_at_history.size();
	}
	
	void UnlockTime() { time_locked = false; }
	
	int CountActiveSizeNow();


  /* functions for Garg's algorithm */
    
  // generate next candidate at random to propose for Garg's algorithm
  int RandGenCand();
  // process global token
  int ProcessGlobalToken();
  // check if candidate queue is empty
  bool IsCandidateQueueEmpty();
  // retrieve next candidate from candidate queue
  Candidate retrieveNextCandidate();
  // insert new candidate into candidate queue
  void insertToCandidateQueue(const Candidate & newCand);
  // check if candidates in buffer has expired according process's my_time_now
  void checkCandidateBuffer();
  // split any active candidate in the candidateBuffer
  void candidateBufferSplit();
};


#endif
