#include <vector>
#include <string>
#include <sstream>
#include <numeric>
#include <random>
#include <algorithm> // for for_each
#include <iomanip>

#include "Process.h"
#include "utility.h"
#include "runsimulation.h"

#include <ctime>

using namespace std;

#define INF 1000000	  //infinity


/*
  Define and assigned global variable in main.cpp
*/

int number_of_processes = 10;   // number of processes/nodes

int delta = 1;          // minimum message delay      
int epsilon = 100;       // uncertainty windows
int delta_max = delta;  
double alpha = 0.05;     // message rate

int run_up_to = 100000; // total number of physical clock cycles in simulation
int absolute_time;      // index of physical clock cycles

distance_matrix D;
probability_matrix prob_matrix;
string topology_file_name;
string prob_matrix_file_name;

// For Garg's algorithm
Token globalToken;
double localPredRate = 0.1;   // randomly generate localPredicate according to this parameter
int num_of_bins = 10;     // number of bin in the historgram
int num_of_runs = 10;       // number of run for each experiments

int log2_max_interval = 5;  // maximum length of interval of a predicate in this case is 2^5 = 32

#ifdef _DEBUG

// for debug Garg's algorithm
ofstream debug_out("debug/wcp_debug_out");
vector<ofstream> debug_candidate;

#endif


// for false positive rate over time
vector<FalsePositiveRateHistory> fHistory;
vector<int> epsilonList = {50, 70, 100, 200, 1000};


/* utility function for paring commandline argument */

enum optionCode{
    DELTA,
    EPSILON,
    ALPHA,
    RUN_UP_TO,
    NUMBER_OF_PROCESSES,
    PROBABILITY_MATRIX_FILE,
    LOCAL_PRED_RATE,
    NUM_OF_BINS,
    NUM_OF_RUNS,
    TOPOLOGY_FILE,
    LOG2_MAX_INTERVAL,
    UNDEFINED
};

optionCode getOptionCode(const string optionString){
    if(optionString == "-d") return DELTA;
    if(optionString == "-e") return EPSILON;
    if(optionString == "-a") return ALPHA;  
    if(optionString == "-u") return RUN_UP_TO;
    if(optionString == "-p") return NUMBER_OF_PROCESSES;
    if(optionString == "-m") return PROBABILITY_MATRIX_FILE;
    if(optionString == "-l") return LOCAL_PRED_RATE;
    if(optionString == "-b") return NUM_OF_BINS;
    if(optionString == "-r") return NUM_OF_RUNS;
    if(optionString == "-t") return TOPOLOGY_FILE;
    if(optionString == "-i") return LOG2_MAX_INTERVAL;
    
    return UNDEFINED;
}

vector<string> splitArg(const string& s){
    vector<string> result;   
    string item1, item2;
    
    item1 = s.substr(0,2);
    result.push_back(item1);
    item2 = s.substr(2,string::npos);
    result.push_back(item2);
    
    //cout << "   item1 = " << item1 << "  item2 = " << item2 << endl;
    
    return result;
}

int parseArg(int argc, char* argv[]){
    int i;
    vector<string> currentArg;
    
    for(i = 1; i < argc; i++){
        //cout << "arg_" << i << " = " << argv[i] << endl;

        currentArg = splitArg(argv[i]);

        
        switch(getOptionCode(currentArg.at(0))){
        case DELTA:
            delta = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "delta = " << delta << endl;
            break;
        case EPSILON:
            epsilon = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "epsilon = " << epsilon << endl;
            break;
        case ALPHA:
            alpha = atof(currentArg.at(1).c_str());
            cout.width(30);
            cout << "alpha = " << alpha << endl;
            break;
        case RUN_UP_TO:
            run_up_to = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "run_up_to = " << run_up_to << endl;
            break;
        case NUMBER_OF_PROCESSES:
            number_of_processes = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "number_of_processes = " << number_of_processes << endl;
            break;
        case PROBABILITY_MATRIX_FILE:
            prob_matrix_file_name = currentArg.at(1);
            cout.width(30);
            cout << "prob_matrix_file_name = " << prob_matrix_file_name << endl;
            break;
        case LOCAL_PRED_RATE:
            localPredRate = atof(currentArg.at(1).c_str());
            cout.width(30);
            cout << "localPredRate = " << localPredRate << endl;
            break;
        case NUM_OF_BINS:
            num_of_bins = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "num_of_bins = " << num_of_bins << endl;
            break;
        case NUM_OF_RUNS:
            num_of_runs = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "num_of_runs = " << num_of_runs << endl;
            break;
        case TOPOLOGY_FILE:
            topology_file_name = currentArg.at(1);
            cout.width(30);
            cout << "topology_file_name = " << topology_file_name << endl;
            break;
        case LOG2_MAX_INTERVAL:
            log2_max_interval = atoi(currentArg.at(1).c_str());
            cout.width(30);
            cout << "log2_max_interval = " << log2_max_interval << endl;
            break;
        default:
            cout << "ERROR: undefined option" << currentArg.at(0) << endl;
            exit(1);
            
        }
        
    }
    
    return 0;
}

/* main function */

int main(int argc, char* argv[]) {
        
    // parsing commandline argument
    parseArg(argc, argv);
    
	// InitAllConfiguration("parameters.in.linux");


    for(int i = 0; i < num_of_runs; i++){  	
        vector<snapshot_diff_count> fullAccumHistogram;
        stringstream stream1, stream2;
        string mystr;    
        clock_t record_time;

        cout << "run " << i << " ..." << endl;
        
        record_time = clock();

 	    vector<int> thisRun = RunSimulation::run(RAND_UNICAST, "Delay-n1000,run1000,alpha0d1,eps100,delta1");
        
        // get simulation time
        record_time = clock() - record_time;
        cout << " time to run simulation: ";
        cout.width(15);
        cout << record_time << " clicks      ";
        cout.width(15);
        cout << ((float)record_time)/CLOCKS_PER_SEC << " seconds " << endl;
        
        // constructing file name according to input parameters
        string accum_hist_filename = string("./results/full_accum_histogram") + string("_p") + to_string(number_of_processes) + string("_d") + to_string(delta);  // histogram
        string fprh_filename = string("./results/fprh") + string("_p") + to_string(number_of_processes) + string("_d") + to_string(delta); // history

        stream1 << fixed << setprecision(3) << alpha;
        mystr = stream1.str();
        accum_hist_filename = accum_hist_filename + string("_a") + mystr;
        fprh_filename = fprh_filename + string("_a") + mystr;

        accum_hist_filename = accum_hist_filename + string("_e") + to_string(epsilon);
        fprh_filename = fprh_filename + string("_e") + to_string(epsilon);            
            
        stream2 << fixed << setprecision(4) << localPredRate;
        mystr = stream2.str();
        accum_hist_filename = accum_hist_filename + string("_l") + mystr;
        fprh_filename = fprh_filename + string("_l") + mystr;

        accum_hist_filename = accum_hist_filename + string("_run") + to_string(i);
        fprh_filename = fprh_filename + string("_run") + to_string(i);

        ofstream full_histogram(accum_hist_filename);
        ofstream fprh_file(fprh_filename);

        // create accumulative histogram
        fullAccumHistogram = Utility::makeFullAccumHistogram(thisRun);        
        // write histogram on output file
        Utility::saveFullAccumHistogram(fullAccumHistogram, full_histogram);


        // save the false positive rate history	
	    Utility::saveFPRH(fHistory, fprh_file);
	    // clear false positive rate history
	    Utility::clearFPRH(fHistory);
        
        // how long to write data onto storage?
        record_time = clock() - record_time;       
        cout << " time to write onto files: ";
        cout.width(15);
        cout << record_time << " clicks      ";
        cout.width(15);
        cout << ((float)record_time)/CLOCKS_PER_SEC << " seconds " << endl;
	    
    }


}

