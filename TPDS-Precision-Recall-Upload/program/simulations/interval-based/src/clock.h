#ifndef CLOCK_H
#define CLOCK_H

/*
  clock.h - classes and functions related to time, clock
            e.g. PhysicalTime, HVC
*/

#include "utility.h"

extern int absolute_time;
extern int epsilon;
extern int delta;
extern int delta_max;

const int HVC_COMPARE_BEFORE = 1;
const int HVC_COMPARE_AFTER = -1;
const int HVC_COMPARE_CONCURRENT = 0;

/*
  PhysicalTime - class for simulation of physical time/clock

	        absolute_time 
    0 -----+-----+-----+-----+-----+-----+-----+-----+-----+-----> run_up_to
            -eps | +eps
            <----+---->
             ^
             |
            PhysicalClock::GetTimeNow() is some randome value within [absolute_time-eps; absolute_time+eps]
             |
             v
            Process::GetTimeNow() = PhysicalClock:GetTimeNow()
             |
             v
            my_time_now = Process:GetTimeNow()

*/

class PhysicalTime {
public:
	static int GetTimeNow() {
		return Utility::GetRandomNumberInRange(absolute_time-epsilon,absolute_time+epsilon);
	}
	
	static int GetDelayMessageTime() {
		return Utility::GetRandomNumberInRange(delta, delta_max);
	}
	
	static int GetAbsoluteTime() { 
	  return absolute_time; 
	}

};


/*
  HVC - class for Hybrid Vector Clock
*/

class HVC {
private:
	int procId;	      // this HVC belongs to which process?
	vector<int> hvc;  // values of hvc
	int active_size;

public:
  // Constructors
  
	HVC(){};
	HVC(const HVC &in){
	  procId = in.procId;
	  hvc = in.hvc;
	  active_size = in.active_size; 
	};

	HVC(int procId, const vector<int> & hvc, int active_size) :procId(procId), hvc(hvc), active_size(active_size) {}

  // gets and sets
  
  vector<int> getHVC() const{
    return hvc;
  }
  
  int getActiveSize(){
    return active_size;
  }
  
  int getProcId(){
    return procId;
  }

  void setHVC(const vector<int> &someHVC){
    hvc = someHVC;
  }
  
  void setHVCElement(int time, int pos){
    hvc.at(pos) = time;
  }
  
  void setActiveSize(int as){
    active_size = as;
  }

  void setProcId(int pid){
    procId = pid;
  }

  int getSize(){
    return hvc.size();
  }

  /*
    Note:
     the comparison operations are vector comparison => VC comparison
     however, since elements in HVC are automatically updated even without communication
     it is HVC comparison.
     if HVC elements are updated with infinite epsilon, HVC are VC, and we have VC comparison

    happenBefore() - get the logical temporal relationship between this HVC and some other HVC
        return value: 1 if this HVC happens before other HVC
                     -1 if other HVC happens before this HVC
                      0 if concurrent (neither one happens before the other)
  */

  int hvcHappenBefore(
    const HVC & they,   // the other HVC to be compared
    int size            // number of elements in vector clock = number of processes
  ){
  
    bool youHappenBefore = true;
    bool theyHappenBefore = true;
    int i;

    for (i = 0; i < size; i++){
      if(hvc.at(i) < they.hvc.at(i))
        theyHappenBefore = false;
      if(hvc.at(i) > they.hvc.at(i))
        youHappenBefore = false;
    }

    /* result
                             theyHappenBefore
                                T     F
                              +----+-----+
                            T | 0  |  1  |
         youHappenBefore      +----+-----+
                            F | -1 |  0  |
                              +----+-----+
    */

    if(youHappenBefore == theyHappenBefore)
      // concurrent
      return HVC_COMPARE_CONCURRENT;

    if(youHappenBefore)
      // you really happen before other HVC
      return HVC_COMPARE_BEFORE;
    else
      // they happens before you
      return HVC_COMPARE_AFTER;

  }

};


class HvcInterval{
private:
  HVC startPoint; // start of interval
  HVC endPoint;   // end of interval

public:
  // empty constructor
  HvcInterval(){};

  // copy constructor
  HvcInterval(const HvcInterval &hi){
    startPoint = hi.startPoint;
    endPoint = hi.endPoint;
  }

  // constructor
  HvcInterval(HVC startPoint, HVC endPoint){
    this->startPoint = startPoint;
    this->endPoint = endPoint;
  }

  HVC getStartPoint(){
    return startPoint;
  }

  HVC getEndPoint(){
    return endPoint;
  }

  int getNumberOfEntries(){
    return startPoint.getSize();
  }

  // comparing two HvcInterval
  int hvcIntervalHappenBefore(HvcInterval other, long twoEpsilon){
    /*
        + Use VC standard

          If interval1.endpoint < interval2.startpoint
              return interval1 -> interval2

          If interval2.endpoint < interval1.startpoint
              return interval2 -> interval1

        + now we have interval1 concurrent to interval 2 in VC metrics
          we will use HVC metrics

          if interval1.endpoint.primaryElement < interval2.startpoint.primaryElement - someEpsilon
            return interval1 -> interval2

          If interval2.endpoint.primaryElement < interval1.startpoint.primaryElement - someEpsilon
            return interval2 -> interval1

          // they are concurrent in both VC and HVC metrics
          return interval2 concurrent interval1
    */

    int hvcSize = startPoint.getSize();

    if(endPoint.hvcHappenBefore(other.startPoint, hvcSize) == HVC_COMPARE_BEFORE){
        return HVC_COMPARE_BEFORE;
    }

    if(startPoint.hvcHappenBefore(other.endPoint, hvcSize) == HVC_COMPARE_AFTER){
        return HVC_COMPARE_AFTER;
    }

  }

};

#endif
