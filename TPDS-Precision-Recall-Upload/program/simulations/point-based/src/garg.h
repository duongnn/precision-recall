/*
  garg.h - data structures for Garg's algorithm
           e.g. Candidate, GlobalToken
*/

#ifndef GARG_H
#define GARG_H

#include <fstream>
#include "clock.h"

using namespace std;

/* 
  constants
*/

// color of token
#define RED   1
#define GREEN 0

// program exit code
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// detection of WCP
#define WCP_DETECTED          1
#define WCP_NOT_YET_DETECTED  0

// function execution return
#define OK  0
#define ERR 1

// global snapshot consistency
#define CONSISTENT      0
#define NOT_CONSISTENT  1

extern int number_of_processes;

/*
  Candidate - class for local state proposed by a process for global snapshot at that process
*/

class Candidate{
private:
  int procId;     // this candidate belongs to which process
  int candId;     // ID of this candidate among the process's proposals
                  // candId = 0 is for initialization in global token
                  // candidates created by process should start with 1

  HVC timestamp;  // timestamp of the candidate (should support both HVC and vector clock)

public:
  // default constructor should not be called since its candId is not managed  
  Candidate(){}

  // prefered constructor
  Candidate(int pid, int cid, const HVC & someHvc){
    procId = pid;
    candId = cid;
    timestamp = someHvc;
  }
  
  // gets and sets

  int getProcId() const {
    return procId;
  }

  int getCandId() const {
    return candId;
  }

  HVC getTimestamp() const {
    return timestamp;
  }

  void setProcId(int pid){
    procId = pid;
  }

  void setCandId(int cid){
    candId = cid;
  }

  void setTimestamp(HVC ts){
    timestamp = ts;
  }

  // print information
  void print(ofstream &);

};
 
 
/*
  Token - class for global token that will be passed around red processes
*/

class Token{
private:
  int pid;    // ID of process who will have this token
              // pid >= 0: color of process pid is red. That process will have the token
              // pid = -1 to signal all processes are green

  // snapshot of the system = an array of candidates proposed by all processes
  // snaphost.at(i) is for candidate from process i
  // init:  snapshot.at(i): procId = i; candId = 0;
  //        HVC for all elements = 0 except element i = 1 or epsilon
  //        or HVC for all elements = 0
  vector<Candidate> snapshot;

  // color array to see if a process is red or green
  // color.at(i) is color for process i
  // init: all colors = RED
  vector<int> color;

public:
  // gets and sets
  
  int getPid() const {
    return pid;
  }
  vector<Candidate> getSnapshot() const {
    return snapshot;
  }
  vector<int> getColor() const {
    return color;
  }
  
  void setPid(int p){
    pid = p;
  }
  
  void setSnapshot(const vector<Candidate> &sns){
    snapshot = sns;
  }
  
  void setProcessCandidate(const Candidate cand, int whichProcess){
    snapshot.at(whichProcess) = cand;
  }
  
  void setColor(const vector<int> cl){
    color = cl;
  }
  
  void setProcessColor(const int cl, int whichProcess){
    color.at(whichProcess) = cl;
  }

  /*
    Token::getMaxTimeDiff() - get the maximum difference in HVC time between any pair of processes in the snapshot
  */

  int getMaxTimeDiff() const {
    int i;
    int max, min;

    // compute maximum physical time difference between all pairs of processes within the snapshot
    max = min = snapshot.at(0).getTimestamp().getHVC().at(0);
    for(i = 1; i < number_of_processes; i++){
      int temp = snapshot.at(i).getTimestamp().getHVC().at(i);

      if(max < temp)
        max = temp;
      if(min > temp)
        min = temp;
    }
    
    return (max-min);
    
  }


  
  // a comparison operator between snapshot
  // return true if your maximum time difference in your snapshot is smaller
  //        false otherwise
  bool operator < (const Token& they) const {
    return getMaxTimeDiff() < they.getMaxTimeDiff();
  }
  
  int checkPhysicalConsistent(int epsilon);
  void printToken(ofstream & os);
  void reset();
  void resetSmallest();
};


#endif

