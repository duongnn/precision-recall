
#include <random>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>

using namespace std;

#include "Process.h"


#ifdef _DEBUG_INTERVAL
ofstream interval_debug_file("./debug/interval_debug_file");
#endif

//#define mdebugfile cout

#define mdebugfile bitBucket

ostream bitBucket(0);

/* send message to a set of processes */

void Process::BroadcastMsg(vector<Process>& vp) {
	for (int k = 0; k < number_of_processes; k++) {
		if (k != id) SendMsg(vp[k]);
	}
}

/* send message to a process */

void Process::SendMsg(Process& k)  {
	
	my_time_now = GetTimeNow();
	RefreshClock();

	if (Utility::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
        // D is distance matrix
		int distance = D[id][k.id];

		int time_delay_msg = 0;
		for (int i = 0; i < distance; i++) {
			time_delay_msg += PhysicalTime::GetDelayMessageTime();
		}
		CountActiveSize();
		int as = CountActiveSizeNow();
		augmented_time.setActiveSize(as);
		k.PushMsg(message(time_delay_msg + absolute_time, augmented_time, id));
		
		// for interval variation of Garg's algorithm
		// since you send a message you need to split any current interval that is active
		firstflag = true;
	}
	
	
}

void Process::CountActiveSize()  {

	int count_active = 0;
	for (int i = 0; i < augmented_time.getHVC().size(); i++) {
		if (augmented_time.getHVC()[i] > my_time_now - epsilon) {
			count_active++;
		}
	}

  // Duong: perhaps we only push size of active count into history 
  //        after we have passed the uncertainty period (i.e. epsilon)

	if(absolute_time > epsilon) size_of_at_history.push_back(count_active);
}

int Process::CountActiveSizeNow()  {

	my_time_now = GetTimeNow();
	
	RefreshClock();
	int count_active = 0;
	for (int i = 0; i < augmented_time.getHVC().size(); i++) {
		if (augmented_time.getHVC()[i] > my_time_now - epsilon) {
			count_active++;
		}
	}
	return count_active;
}


void Process::ReceiveMsg(){

    if (!mail_box.empty() && begin(mail_box)->arrival_time <= PhysicalTime::GetAbsoluteTime()) {

        int size_now = CountActiveSizeNow();

        while (!mail_box.empty() && begin(mail_box)->arrival_time <= PhysicalTime::GetAbsoluteTime()) {
	        auto m_itr = begin(mail_box);
            // if (augmented_time.active_size <= size_now)
            {

                vector<int> m_at = m_itr->sender_at.getHVC(); // should be HVC instead of vector<int>?
                for (int i = 0; i < (int)m_at.size(); i++) {
                    //augmented_time.getHVC()[i] = max(m_at[i], augmented_time.getHVC()[i]);
                    augmented_time.setHVCElement(max(m_at[i], augmented_time.getHVC()[i]), i);

                }
            }

	        mail_box.erase(m_itr);
        }
        my_time_now = GetTimeNow();
        RefreshClock();
        CountActiveSize();
        
        // turn on firstflag
        firstflag = true;
    }
	
}

void Process::PushMsg(const message& m){
	mail_box.insert(m);
}

int Process::GetTimeNow() const {
  
	if (time_locked) return my_time_now;
	
	return max(my_time_now+1,PhysicalTime::GetTimeNow());

}

// RefreshClock is called whenever process sends or receives messages

void Process::RefreshClock() {

	if (time_locked) return;
	
	for (int i = 0; i < (int)augmented_time.getHVC().size(); i++) {
		if (i == id) continue;
		if (augmented_time.getHVC()[i] < my_time_now - epsilon ) {
			augmented_time.setHVCElement(my_time_now - epsilon, i);
		}
	}
	
	augmented_time.setHVCElement(my_time_now, id);
	
	time_locked = true;
}

// For Garg's algorithm

/*
  Process:RandGenCand() - randomly generate a candidate:
    randomly generate a number.
    If the number <= pre-determined rate
      Create new candidate at current timestamp
      Push the candidate into candiate queue
    Else
      do nothing
*/

int Process::RandGenCand(){

    
    // cout << "RandGenCand at process " << id << " my_time_now = " << my_time_now << " absolute_time = " << absolute_time << endl << std::flush;

    // you should only generate new candidate if firstflag is true
    if(firstflag){
    
        //cout << " firstflag == TRUE " << std::flush;
    
        if(!candidateBuffer.empty()){

            //cout << ": SPLIT " << endl << std::flush;

            // there is active candidate and firstflag == true
            // we need to split the active candidate

            candidateBufferSplit();
            
            // firstflag = false; // this action should be done in candidateBufferSplit()
            
            
        }else{
            // there is no active candidate
            // firstflag == true, so we can generate new candidate with predefined probability
            // if candidate is generated, put it into candidateBuffer first, and mark firstflag = false

            //cout << ": Generate new candidate: " << std::flush;

            if(Utility::GetRandomNumberInRange(1, 100)*1.0 <= localPredRate * 100){
                // randomly generate interval for the candidate such that 
                // the longer the interval, the less likely it is generated
                // the range of interval_size is scaled up to be greater than 100
                //int interval_size = 100 + Utility::GetRandomNumberGeometricDistribution(geometric_dist_param) * 10;
                // int interval_size = 10 + Utility::GetRandomNumberGeometricDistribution(geometric_dist_param) * 10;

                // to make the simulation match with the analytical model
                // the interval size is fixed instead of following geometric distribution
                // the interval size can be split when there is a communication in the middle
                int interval_size = base_interval_length;
                
                candidateIntervalSizeList.push_back(interval_size);
                
                Candidate newCand = Candidate(id, ++candCount, augmented_time, interval_size);
                
                candidateBuffer.push(newCand);
                
                firstflag = false;
                

                #ifdef _DEBUG

                // print candidate to debug file
                newCand.print(debug_candidate.at(id));
                debug_candidate.at(id) << endl;

                #endif

                //cout << ": YES " << endl;

            }else{
                //cout << ": NO " << endl;
            }
            
        } // end of if candidateBuffer.empty()
    
    }else{
        // just do nothing
        
        //cout << "firstflag == FALSE " << endl << std::flush;
        
    } // end of if firstflag
    
    /* 
    if firstflag == true
        if there is active candidate interval (in candidate buffer queue)
            split the candidate's into 2 candidates (the interval is splittible when its last endpoint >= my_time_now)
                old candidate after split will expire, thus move to candidateQueue
                new candidate is move to candidate buffer queue
            firstflag = false
        else // currently no active candidate (i.e. all have expired)
            generate new one with probability of localPredRate
            if success
                put the candidate into candidateBuffer
                firstflag = false // i.e. some candidate is being active
            else
                do nothing, just leave firstflag = true
    else // firstflag == false
        // firstflag == flase => this assume there is some active candidate still in the buffer
        //                       you should not generate a new one, otherwise they will overlap
        just return
                
    */
    

    return 1;
}


/*
  Process:ProcessGlobalToken() - process the global token if that token is at your place right now
                                 i.e. token's ID matches your ID  
      + retrieve next candidate until its color in the token becomes green
        update color for other process accordingly
        and change value of token accordingly (to pass token to other process)
      + if candidate runs out, it just return, but doesn't change value of token
        so that it will be able to process the token in the next run

      + return  WCP_DETECTED if global predicate is found
                WCP_NOT_YET_DETECTED otherwise
*/

int Process::ProcessGlobalToken(){
    bool found_valid_candidate = false;
    bool all_green;
    int whoIsNext;
    
    // now you are red, i.e. globalToken.color.at(id) = RED
    while(!IsCandidateQueueEmpty()){
        bool happen_before = false;
        int  comparison;
        Candidate nextCand = retrieveNextCandidate();

        // replace old candidate by the new one
        globalToken.setProcessCandidate(nextCand, id);

        for(int i = 0; i < number_of_processes; i++){
            if(i != id){
                // compare your candidate HVC with other candidate's HVC
                comparison = nextCand.getTimestamp().hvcHappenBefore(globalToken.getSnapshot().at(i).getTimestamp(), number_of_processes);

                switch(comparison){
                case 1:
                    // your HVC happens before other candidte's HVC
                    happen_before = true;
                    break;
                case -1:
                    // other candidate's HVC happens before you
                    // mark other candidate's color as red
                    globalToken.setProcessColor(RED, i);

                    break;
                default: // concurrent
                    break;
                }

                // no need to continue if you know you happen before some other's candidate
                // need to retrieve new candidate
                if(happen_before)
                    break;
            }
        } // for (i = 0 ...

        if(happen_before)  // this candidate is not valid
            continue;        // get next candidate if possible
        else{              // valid candidate
            // your color is GREEN
            globalToken.setProcessColor(GREEN, id);
            found_valid_candidate = true;

            // no need to retrieve next candidate
            break;
        }
    } //   while(!IsCandidateQueueEmpty())

    if(found_valid_candidate){
        // you found a valid candidate
        // check if you are the last one that is green
        all_green = true;

        for (int i = 0; i < number_of_processes; i++){
            if(globalToken.getColor().at(i) == RED){
                all_green = false;
                whoIsNext = i;
                break;
            }
        }

        if(!all_green){
            // there is still some red
            globalToken.setPid(whoIsNext);

        }else{
            // all are green: global predicate is found
            globalToken.setPid(-1);


            return WCP_DETECTED;
        }

    }else{
        // mark globalToken.pid be yourself so that you will continue to hold it at next clock cycle
        globalToken.setPid(id);

        // make sure you are still red
        globalToken.setProcessColor(RED, id);

    } // if (found_valid_candidate)


    // return for considering next candidate
    return WCP_NOT_YET_DETECTED;

}

// check if candidate queue is empty
bool Process::IsCandidateQueueEmpty(){
    return candidateQueue.empty();
}

// retrieve next candidate from candidate queue
Candidate Process::retrieveNextCandidate(){
  Candidate result;

  if(!IsCandidateQueueEmpty()){
    result = candidateQueue.front();
    candidateQueue.pop();
    return result;
  }

  // return NULL;
}

// insert a candidate into candidate queue
void Process::insertToCandidateQueue(const Candidate & newCand){
  candidateQueue.push(newCand);
  
  // just for debug
  candidateReadyIntervalSizeList.push_back(newCand.getIntervalSize());
  
}

// check if candidate in the queue has expired. If yes, move it to candidateQueue
void Process::checkCandidateBuffer(){
    
    if(candidateBuffer.empty()){
        // make sure firstflag is turned to true
        firstflag = true;
        return;
    }

    #ifdef _DEBUG_INTERVAL
    if(candidateBuffer.size() > 1){
        // something wrong since candidateBuffer should hold at most 1 active candidate
        // more than 1 active candidate will cause overlapping
        
        mdebugfile << " ERROR Process::checkCandidateBuffer() number of active candidate > 1: " << candidateBuffer.size() << endl;
        
        // terminate the program for debugging
        exit(1);
    }
    #endif
    
    while(!candidateBuffer.empty()){
        // get the oldest candidate in buffer
        Candidate a_cand = candidateBuffer.front();
        
        
        // check if this candidate has expired
        if(a_cand.getTimestamp().getHVC().at(GetId()) + a_cand.getIntervalSize() - 1 >= my_time_now){
            // not expired yet
            break;
        }else{
            // has expired, push to candidateQueue
            insertToCandidateQueue(a_cand);
            // pop out this candidate
            candidateBuffer.pop();
            
        }
    }
    

    if(candidateBuffer.empty()){
        // make sure firstflag is turned to true
        firstflag = true;
        return;
    }else{
        // there is still active candidate, but should not make firstflag = false
        // since firstflag may have been turned be true by communication event
        
        return;
    }

}

void Process::candidateBufferSplit(){


    #ifdef _DEBUG_INTERVAL
    interval_debug_file << " my_time_now = " << my_time_now << endl << std::flush;
    
    interval_debug_file << " Process " << GetId() << " candidateBufferSplit(): at beginning: " << endl << std::flush;
    interval_debug_file << " candidateBuffer.size() = " << candidateBuffer.size() << endl << std::flush;
    if(candidateBuffer.size() > 0){
        interval_debug_file << " front: " << endl << std::flush;
        candidateBuffer.front().print(interval_debug_file);
        interval_debug_file << endl << std::flush;
    }
    interval_debug_file << " candidateQueue.size() = " << candidateQueue.size() << endl << std::flush;
    if(candidateQueue.size() > 0){
        interval_debug_file << " front: " << endl << std::flush;
        candidateQueue.front().print(interval_debug_file);
        interval_debug_file << endl << std::flush;   
    
        interval_debug_file << " back: " << endl << std::flush;
        candidateQueue.back().print(interval_debug_file);
        interval_debug_file << std::flush;
    }
    #endif
    
    #ifdef _DEBUG_INTERVAL
    if(candidateBuffer.size() > 1){
        // something wrong since candidateBuffer should hold at most 1 active candidate
        // more than 1 active candidate will cause overlapping
                
        // terminate the program for debugging
        exit(1);
    
    }
    #endif
    
    if(candidateBuffer.empty()){
        // this should not happen since candidateBufferSplit() is invoked 
        // only when candidateBuffer is not empty and firstflag == true
        return;
    }

        
    // get the oldest candidate in buffer
    Candidate first_cand = candidateBuffer.front();
        
    // check if this candidate has expired
    if(first_cand.getTimestamp().getHVC().at(GetId()) + first_cand.getIntervalSize() - 1 >= my_time_now){

        mdebugfile << " candidate still active => split " << endl << std::flush;
        mdebugfile << " my_time_now = " << my_time_now << endl << std::flush;
        mdebugfile << " candidate: " << endl << std::flush;
        
        //first_cand.print(cout);
        
        mdebugfile << endl << std::flush;

        // not expired yet: good => split it
        
        int first_interval_size, second_interval_size;
                    
        // for first candidate: update its interval, pop it out, and move it to candidateQueue
        // interval size should not include my_time_now
        
        first_interval_size = my_time_now - first_cand.getTimestamp().getHVC().at(GetId());
        second_interval_size = first_cand.getIntervalSize() - first_interval_size;

        mdebugfile << " start time = " << first_cand.getTimestamp().getHVC().at(GetId())
             << " my_time_now = " << my_time_now
             << " first_interval_size = " << first_interval_size
             << " second_interval_size = " << second_interval_size << endl << std::flush;
        

        first_cand.setIntervalSize(first_interval_size);
        candidateBuffer.pop();
        insertToCandidateQueue(first_cand);
        
        // for second candidate: update its CandId, timestamp, interval size
        // then push it into this candidateBuffer
        // since sendMsg or receiveMsg would update augmented_time, augmented_time will reflect most updated time
                    
        Candidate second_cand = Candidate(GetId(), ++candCount, augmented_time, second_interval_size);

        candidateBuffer.push(second_cand);
        
        // candidateBuffer has active element, thus firstflag should be turned to false
        firstflag = false;
        
    }else{
        // has expired, push to candidateQueue: somethingwrong => printout error message for debugging

        cout << " candidate expires => ERRRRROOOOOOOOOORRR " << endl << std::flush;

        insertToCandidateQueue(first_cand);
        
        #ifdef _DEBUG_INTERVAL
        cout << " ERROR: Process::CandidateBufferSplit() the candidate cannot be split since it has expired " << endl;
        cout << " my_time_now = " << my_time_now << endl;
        cout << " candidate: "<< endl;
        //first_cand.print(cout);
        mdebugfile << endl;
        exit(1);
        #endif
        
    }

    


    #ifdef _DEBUG_INTERVAL
    interval_debug_file << " Process " << GetId() << " candidateBufferSplit(): at return: " << endl << std::flush;

    interval_debug_file << " candidateBuffer.size() = " << candidateBuffer.size() << endl << std::flush;
    if(candidateBuffer.size() > 0){
        interval_debug_file << " front: " << endl << std::flush;
        candidateBuffer.front().print(interval_debug_file);
        interval_debug_file << endl << std::flush;
    }
    interval_debug_file << " candidateQueue.size() = " << candidateQueue.size() << endl << std::flush;
    if(candidateQueue.size() > 0){
        interval_debug_file << " front: " << endl << std::flush;
        candidateQueue.front().print(interval_debug_file);
        interval_debug_file << endl << std::flush;   
    
        interval_debug_file << " back: " << endl << std::flush;
        candidateQueue.back().print(interval_debug_file);
        interval_debug_file << std::flush;
    }

    #endif


}
