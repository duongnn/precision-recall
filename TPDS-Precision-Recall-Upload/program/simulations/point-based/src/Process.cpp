
#include <random>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>

using namespace std;

#include "Process.h"


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

  //cout << "RandGenCand at process " << id << " my_time_now = " << my_time_now << " absolute_time = " << absolute_time;


  // create a new candidate and push it into the queue
  if(Utility::GetRandomNumberInRange(1, 100)*1.0 <= localPredRate * 100){
    Candidate newCand = Candidate(id, ++candCount, augmented_time);
    candidateQueue.push(newCand);

    #ifdef _DEBUG

    // print candidate to debug file
    newCand.print(debug_candidate.at(id));
    debug_candidate.at(id) << endl; 

    #endif

    //cout << ": YES " << endl;

  }else{
    //cout << ": NO " << endl;
  }

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
        comparison = nextCand.getTimestamp().happenBefore(globalToken.getSnapshot().at(i).getTimestamp(), number_of_processes);

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

// insert new candidate into candidate queue
void Process::insertNewCandidate(const Candidate & newCand){
  candidateQueue.push(newCand);
}


