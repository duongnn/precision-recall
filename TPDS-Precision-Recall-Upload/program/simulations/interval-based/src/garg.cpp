/*
  garg.cpp - definition of member methods of classes in garg.h
*/

#include <sstream>
#include "garg.h"


// utitlity method to print content of a candidate on a line to an output stream
//void Candidate::print(ofstream &os){
void Candidate::print(ostream &os){
  int i;

  os << "(";
  os.width(5); os << procId; 
  os << ", ";
  os.width(6); os << candId; 
  os << ", [";
  os.width(6); 
  os << intervalSize << "]):" << std::flush;

  for(i = 0; i < number_of_processes; i++){
    os.width(10);

    os << timestamp.getHVC().at(i);
  }

}


// utility method to print content of a token to an output stream
void Token::printToken(ofstream & os){
  int i;
  int max, min;

  // first line is token's pid, should be -1 for token with consistent snapshot
  os << pid << endl;

  updateEndPoints();
  
  os << "intervalStartDiff = " << getIntervalStartDiff() << "   intervalMinDiff = " << getIntervalMinDiff() << endl; 
    
  // each process's candidate is a line    
  for(i = 0; i < number_of_processes; i++){

    switch(color.at(i)){
      case GREEN:
        os << "[GREEN] "; break;
      case RED:
        os << "[RED]   "; break;
    }

    snapshot.at(i).print(os) ;
    os << endl;
    
  }
}

/*
  Token::checkPhysicalConsistent()
     -  check whether this token is consistent with physical clock 
        given a certainty epsilon
     -  return  CONSISTENT if it is consistent with physical clock, i.e. all pair of HVC should overlap
                NOT_CONSISTENT otherwise 
*/

int Token::checkPhysicalConsistent(int epsilon){
  int max, min; // maximum and minimum HVC
  int i;
  int size = snapshot.size();
  
  max = min = snapshot.at(0).getTimestamp().getHVC().at(0);
  
  for(i = 1; i < size; i++){
    int temp = snapshot.at(i).getTimestamp().getHVC().at(i);
    if (temp < min)
      min = temp;
    if (temp > max)
      max = temp;
    // max - min is largest difference between 2 HVCs so far
    if(max - min > 2*epsilon)
      return NOT_CONSISTENT;
    
  }
  
  return CONSISTENT;
}


/*
  Token::reset() - reset a token, i.e. invalidate the token so that we can find next snapshots
*/

void Token::reset(){
  // mark all candidate as invalid
  setColor(vector<int>(number_of_processes, RED));

  // process 0 is next process to have token  
  setPid(0);
}

/*
    Token:resetSmallest() - reset the flag of smallest process
*/

void Token::resetSmallest(){
    int smallestPid;
    int smallestClock;
    int i;
    
    
    // find the process with smallest physical clock
    smallestPid = 0;
    smallestClock = snapshot.at(0).getTimestamp().getHVC().at(0);
    
    for(i = 1; i < number_of_processes; i++){
        int temp = snapshot.at(i).getTimestamp().getHVC().at(i);
        
        if(temp < smallestClock){
            smallestClock = temp;
            smallestPid = i;
        }
    }
    
    // mark its color as RED
    setProcessColor(RED, smallestPid);
    
    // mark that process as the next one
    setPid(smallestPid);
}

