#ifndef RUN_SIMULATION_H
#define RUN_SIMULATION_H

#include <vector>
#include <string>
#include <sstream>
#include <numeric>
#include <random>
#include <algorithm> // for for_each

#include "Process.h"

#define RAND_UNICAST 1

using namespace std;  

extern int number_of_processes;   // number of processes/nodes

extern int delta;          // minimum message delay      
extern int epsilon;       // uncertainty windows
extern int delta_max;  
extern double alpha;     // message rate

extern int run_up_to; // total number of physical clock cycles in simulation
extern int absolute_time;      // index of physical clock cycles

extern probability_matrix prob_matrix;

extern int snapshotCount;
extern vector<Token> snapshotAll;

extern vector<int> intervalStartDiffList;
extern vector<int> intervalMinDiffList;
extern vector<int> intervalStartMinDiffList; // records of difference between StardDiff and MinDiff
extern vector<int> intervalStartMinDiffPercentList; // records of difference between StartDiff and MinDiff in percentage


//extern vector<FalsePositiveRateHistory> fHistory;
extern vector<FalsePositiveRateHistory> fHistory_intervalStartDiff;    // for intervalStartDiff
extern vector<FalsePositiveRateHistory> fHistory_intervalMinDiff;      // for intervalMinDiff

extern vector<int> epsilonList;

extern ofstream debug_out;

class RunSimulation{
public:
  static vector<Process> InitProcess();
  static void run(int type, const string& s);
  static void RandomUnicastExperimentSnapshot(const string& s);
};

#endif
