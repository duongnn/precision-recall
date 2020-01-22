/*
  garg.h - data structures for Garg's algorithm
           e.g. Candidate, GlobalToken
*/

#ifndef GARG_H
#define GARG_H

#include <fstream>
#include "clock.h"

using namespace std;

// 
//  constants
//

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

// Cut consistency
// #define CONSISTENT_CUT (-1)     // Currently we don't need the class Cut

extern int number_of_processes;

//
//  Candidate - class for local state proposed by a process for global snapshot at that process
//

class Candidate{
private:
  int procId;     // this candidate belongs to which process
  int candId;     // ID of this candidate among the process's proposals
                  // candId = 0 is for initialization in global token
                  // candidates created by process should start with 1
//  int sequenceId; // to differ candidate of the same interval, starting from 1
  
  // example:
  // time           |----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
  //
  //                          pred 1, intervalSize=2   pred 2, intervalSize=4   pred 3, intervalSize=1
  // process 3                +----+                   +----+----+----+         +
  // Candidate procId         3    3                   3    3    3    3         3
  //           candId         1    1                   2    2    2    2         3
  //           sequenceId     1    2                   1    2    3    4         1

  HVC timestamp;    // timestamp of the candidate (should support both HVC and vector clock)
  int intervalSize; // how many points in the interval, i.e. how long the predicate is true. Must >= 1

public:

    // default constructor should not be called since its candId is not managed  
    Candidate(){}

    // prefered constructor
//    Candidate(int pid, int cid, int sid, const HVC & someHvc, int is){
    Candidate(int pid, int cid, const HVC & someHvc, int is){
        procId = pid;
        candId = cid;
//        sequenceId = sid;
        timestamp = someHvc;
        intervalSize = is;
    }

    // copy constructor
    Candidate(const Candidate &a_candidate){
        procId = a_candidate.getProcId();
        candId = a_candidate.getCandId();
        timestamp = a_candidate.getTimestamp(); // check if this work or HVC(a_candidate.getTimestamp())
        intervalSize = a_candidate.getIntervalSize();
        
    }

    
    // gets and sets

    int getProcId() const {
        return procId;
    }

    int getCandId() const {
        return candId;
    }

//    int getSequenceId() const{
//        return sequenceId;
//    }

    HVC getTimestamp() const {
        return timestamp;
    }

    int getIntervalSize() const{
        return intervalSize;
    }

    void setProcId(int pid){
        procId = pid;
    }

    void setCandId(int cid){
        candId = cid;
    }

//    void setSequenceId(int sid){
//        sequenceId = sid;
//    }

    void setTimestamp(HVC ts){
        timestamp = ts;
    }

    void setIntervalSize(int is){
        intervalSize = is;
    }

    // print information
    //void print(ofstream &);
    void print(ostream &);


};
 
 
//
//  Token - class for global token that will be passed around red processes
//

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

    int firstStart;   // timestamp of the begining of the interval that start first
    int lastStart;    // timestamp of the begining of the interval that start last
    int firstEnd;     // timestamp of the ending of the interval that ends first
    int lastEnd;      // timestamp of the ending of the interval that ends last  
    // from those four values we can obain the 2 values
    //    1. the maximum difference between the beginning points of all interval
    //            = lastStart - firstStart
    //    2. The minimal of maximal difference between intervals, as described below:
    //    Suppose we have the snapshot of intervals consistent according to VC (happen-before relation).
    //    Now we draw a line (which may be not straight but curvy) through the intervals.
    //    That line intersects with the intervals at different points.
    //    Clearly, there is the maximum difference between all such interseting points of the line, i.e,
    //    the difference between the right-most point and the left-most point of the line.
    //    What is the minimum possible value for that maximum difference of such a line cutting through
    //    the interval?
    //          if firstEnd >= lastStart
    //              it is equal to 0      // since we can make a straight vertical line cutting through the intervals
    //          else
    //              it is equal to lastStart - firstEnd   // we can't make a vertical line but only curvy lines
    //    That minimum of maximum difference of intervals (of the snapshot, of the cutting line) will determine whether
    //    that snapshot is still consistent or not under HVC criteria.
    //    In particular, if that minimum of maximum difference <= eps, then it is HVC consistent
    //                   otherwise, it is not HVC consistent
  

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

    int getFirstStart() const{
        return firstStart;
    }
    int getLastStart() const{
        return lastStart;
    }
    int getFirstEnd() const{
        return firstEnd;
    }
    
    int getLastEnd() const{
        return lastEnd;
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
  
    // update the value of firstStart, lastStart, firstEnd, lastEnd
    void updateEndPoints(){
        int i;
        int start, end;

        // get start and end point of first interval
        start = snapshot.at(0).getTimestamp().getHVC().at(0);
        end = start + snapshot.at(0).getIntervalSize() - 1;

        // assume you have one interval
        firstStart = lastStart = start;
        firstEnd = lastEnd = end;

        for(i = 1; i < number_of_processes; i++){
            // get start and end point of next interval
            start = snapshot.at(i).getTimestamp().getHVC().at(i);
            end = start + snapshot.at(i).getIntervalSize() - 1;
            
            // update value if needed
            if(firstStart > start)
                firstStart = start;
            if(lastStart < start)
                lastStart = start;
            if(firstEnd > end)
                firstEnd = end;
            if(lastEnd < end)
                lastEnd = end;
            
        }

    }
    
    // get the maximum difference between all intervals' starting point
    int getIntervalStartDiff()const{
        return lastStart - firstStart;
    }
    
    // get the minimum possible of the maximum difference within a cut through the intervals
    int getIntervalMinDiff() const{
        if (firstEnd >= lastStart)
            return 0;
        else
            return lastStart - firstEnd;    
    }

    

    //
    // Token::getMaxTimeDiff() - get the maximum difference in HVC time between any pair of processes in the snapshot
    //

//  int getMaxTimeDiff() const {
//    int i;
//    int max, min;

//    // compute maximum physical time difference between all pairs of processes within the snapshot
//    max = min = snapshot.at(0).getTimestamp().getHVC().at(0);
//    for(i = 1; i < number_of_processes; i++){
//      int temp = snapshot.at(i).getTimestamp().getHVC().at(i);

//      if(max < temp)
//        max = temp;
//      if(min > temp)
//        min = temp;
//    }
//    
//    return (max-min);
//    
//  }


  
  // a comparison operator between snapshot
  // return true if your maximum time difference in your snapshot is smaller
  //        false otherwise
//  bool operator < (const Token& they) const {
//    return getMaxTimeDiff() < they.getMaxTimeDiff();
//  }
  
    int checkPhysicalConsistent(int epsilon);
    void printToken(ofstream & os);
    void reset();
    void resetSmallest();

};


#endif

