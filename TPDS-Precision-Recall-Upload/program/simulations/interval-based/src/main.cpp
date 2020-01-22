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

using namespace std;

#define INF 1000000	  //infinity


/*
  Define and assign default values for global variables in main.cpp
*/


int number_of_processes = 10;   // number of processes/nodes

int delta = 1;          // minimum message delay      
int epsilon = 100;       // uncertainty windows
int delta_max = delta;  
double alpha = 0.05;     // message rate
int base_interval_length = 10;  // base for size of interval, i.e. interval size = base + some number in geometric distribution

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
double geometric_dist_param = 0.3;  // parameter of geometric distribution

string delta_str;
string epsilon_str;
string alpha_str;
string run_up_to_str;
string number_of_processes_str;
string localPredRate_str;
string num_of_bins_str;
string num_of_runs_str;
string log2_max_interval_str;
string geometric_dist_param_str;
string base_interval_length_str;

#ifdef _DEBUG
// for debug Garg's algorithm
ofstream debug_out("debug/wcp_debug_out");
vector<ofstream> debug_candidate;
#endif


// for false positive rate history over time

vector<int> intervalStartDiffList;  // records of maximum difference of intervals' start
vector<int> intervalMinDiffList;    // records of minimum of the maximum difference of cuts through the intervals

vector<int> intervalStartMinDiffList; // records of difference between StardDiff and MinDiff
vector<int> intervalStartMinDiffPercentList; // records of difference between StartDiff and MinDiff in percentage

vector<FalsePositiveRateHistory> fHistory_intervalStartDiff;    // for intervalStartDiff
vector<FalsePositiveRateHistory> fHistory_intervalMinDiff;      // for intervalMinDiff
vector<int> epsilonList = {50, 100, 200, 600, 1000};


// for debugging how many candidate with length > 1 are generated
vector<int> candidateIntervalSizeList;
vector<int> candidateReadyIntervalSizeList;


/* utility function for parsing commandline argument */

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
    GEOMETRIC_DIST_PARAM,
    BASE_INTERVAL_LENGTH,
    UNDEFINED
};

// optionCode getOptionCode(const string optionString){
//     if(optionString == "-d") return DELTA;
//     if(optionString == "-e") return EPSILON;
//     if(optionString == "-a") return ALPHA;  
//     if(optionString == "-u") return RUN_UP_TO;
//     if(optionString == "-p") return NUMBER_OF_PROCESSES;
//     if(optionString == "-m") return PROBABILITY_MATRIX_FILE;
//     if(optionString == "-l") return LOCAL_PRED_RATE;
//     if(optionString == "-b") return NUM_OF_BINS;
//     if(optionString == "-r") return NUM_OF_RUNS;
//     if(optionString == "-t") return TOPOLOGY_FILE;
//     if(optionString == "-i") return LOG2_MAX_INTERVAL;
//     if(optionString == "-g") return GEOMETRIC_DIST_PARAM;
    
//     return UNDEFINED;   // cfhjknoqsvwxyz
// }

// vector<string> splitArg(const string& s){
//     vector<string> result;   
//     string item1, item2;
    
//     item1 = s.substr(0,2);
//     result.push_back(item1);
//     item2 = s.substr(2,string::npos);
//     result.push_back(item2);
    
//     //cout << "   item1 = " << item1 << "  item2 = " << item2 << endl;
    
//     return result;
// }

optionCode getLongOptionCode(const string longOptionString){
    if(longOptionString == "--delta") return DELTA;
    if(longOptionString == "--epsilon") return EPSILON;
    if(longOptionString == "--alpha") return ALPHA;  
    if(longOptionString == "--run-up-to") return RUN_UP_TO;
    if(longOptionString == "--number-of-process") return NUMBER_OF_PROCESSES;
    if(longOptionString == "--prob-matrix-file") return PROBABILITY_MATRIX_FILE;
    if(longOptionString == "--local-pred-rate") return LOCAL_PRED_RATE;
    if(longOptionString == "--number-of-bins") return NUM_OF_BINS;
    if(longOptionString == "--num-of-runs") return NUM_OF_RUNS;
    if(longOptionString == "--topo-file") return TOPOLOGY_FILE;
    if(longOptionString == "--log2-max-interval") return LOG2_MAX_INTERVAL;
    if(longOptionString == "--geo-dist-param") return GEOMETRIC_DIST_PARAM;
    if(longOptionString == "--base-interval-length") return BASE_INTERVAL_LENGTH;

    // not match any defined one
    return UNDEFINED;
}

vector<string> splitArg(const string& s, char delimiter){
    vector<string> result;
    string argTypeStr, argValueStr;
    std::size_t delimiterPosition;    

    // find the equal sign
    delimiterPosition = s.find(delimiter);

    if(delimiterPosition != string::npos){
        argTypeStr = s.substr(0, delimiterPosition);
        argValueStr = s.substr(delimiterPosition + 1, string::npos);
        result.push_back(argTypeStr);
        result.push_back(argValueStr);

        return result;        

    }else{
        cout << " could not find delimiter " << delimiter << " in string " << s << endl;
        return vector<string>();    // return empty vector
    }
}

int parseArg(int argc, char* argv[]){
    int i;
    vector<string> currentArg;
    
    for(i = 1; i < argc; i++){
        //cout << "arg_" << i << " = " << argv[i] << endl;

        currentArg = splitArg(argv[i], '=');

        switch(getLongOptionCode(currentArg.at(0))){
        case DELTA:
            delta_str = currentArg.at(1);
            delta = atoi(delta_str.c_str());
            cout.width(30);
            cout << "delta = " << delta << endl;
            break;
        case EPSILON:
            epsilon_str = currentArg.at(1);
            epsilon = atoi(epsilon_str.c_str());
            cout.width(30);
            cout << "epsilon = " << epsilon << endl;
            break;
        case ALPHA:
            alpha_str = currentArg.at(1);
            alpha = atof(alpha_str.c_str());
            cout.width(30);
            cout << "alpha = " << alpha << endl;
            break;
        case RUN_UP_TO:
            run_up_to_str = currentArg.at(1);
            run_up_to = atoi(run_up_to_str.c_str());
            cout.width(30);
            cout << "run_up_to = " << run_up_to << endl;
            break;
        case NUMBER_OF_PROCESSES:
            number_of_processes_str = currentArg.at(1);
            number_of_processes = atoi(number_of_processes_str.c_str());
            cout.width(30);
            cout << "number_of_processes = " << number_of_processes << endl;
            break;
        case PROBABILITY_MATRIX_FILE:
            prob_matrix_file_name = currentArg.at(1);
            cout.width(30);
            cout << "prob_matrix_file_name = " << prob_matrix_file_name << endl;
            break;
        case LOCAL_PRED_RATE:
            localPredRate_str = currentArg.at(1);
            localPredRate = atof(localPredRate_str.c_str());
            cout.width(30);
            cout << "localPredRate = " << localPredRate << endl;
            break;
        case NUM_OF_BINS:
            num_of_bins_str = currentArg.at(1);
            num_of_bins = atoi(num_of_bins_str.c_str());
            cout.width(30);
            cout << "num_of_bins = " << num_of_bins << endl;
            break;
        case NUM_OF_RUNS:
            num_of_runs_str = currentArg.at(1);
            num_of_runs = atoi(num_of_runs_str.c_str());
            cout.width(30);
            cout << "num_of_runs = " << num_of_runs << endl;
            break;
        case TOPOLOGY_FILE:
            topology_file_name = currentArg.at(1);
            cout.width(30);
            cout << "topology_file_name = " << topology_file_name << endl;
            break;
        case LOG2_MAX_INTERVAL:
            log2_max_interval_str = currentArg.at(1);
            log2_max_interval = atoi(log2_max_interval_str.c_str());
            cout.width(30);
            cout << "log2_max_interval = " << log2_max_interval << endl;
            break;
        case GEOMETRIC_DIST_PARAM:
            geometric_dist_param_str = currentArg.at(1);
            geometric_dist_param = atof(geometric_dist_param_str.c_str());
            cout.width(30);
            cout << "geometric_dist_param = " << geometric_dist_param << endl;
            break;
        case BASE_INTERVAL_LENGTH:
            base_interval_length_str = currentArg.at(1);
            base_interval_length = atoi(base_interval_length_str.c_str());
            cout.width(30);
            cout << "base_interval_length = " << base_interval_length << endl;
            break;
        default:
            cout << "ERROR: undefined option " << currentArg.at(0) << endl;
            exit(1);
            
        }
        
    }
    
    return 0;
}

/* main function */

int main(int argc, char* argv[]) {
        
    // parsing commandline argument
    parseArg(argc, argv);
    

    for(int runId = 0; runId < num_of_runs; runId++){
        vector<snapshot_diff_count> isd_accum_histogram, imd_accum_histogram;
        vector<snapshot_diff_count> ismd_accum_histogram, ismdp_accum_histogram;
        clock_t record_time;
        
        record_time = clock();
        
        cout << "run " << runId << " ..." << endl;

 	    RunSimulation::run(RAND_UNICAST, "Delay-n1000,run1000,alpha0d1,eps100,delta1");
        
        // get simulation time
        record_time = clock() - record_time;
        cout << " time to run simulation: ";
        cout.width(15);
        cout << record_time << " clicks      ";
        cout.width(15);
        cout << ((float)record_time)/CLOCKS_PER_SEC << " seconds " << endl;

        // constructing file name according to input parameters
        string isd_filename = string("./results/isd");
        string imd_filename = string("./results/imd");
        string ismd_filename = string("./results/ismd");
        string ismdp_filename = string("./results/ismdp");
        
        string isd_fpr_filename = string("./results/isd_fpr");
        string imd_fpr_filename = string("./results/imd_fpr");

        isd_filename = isd_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);
        imd_filename = imd_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);

        ismd_filename = ismd_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);
        ismdp_filename = ismdp_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);

        isd_fpr_filename = isd_fpr_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);
                        
        imd_fpr_filename = imd_fpr_filename + string("_p") + number_of_processes_str + string("_a") + alpha_str
                        + string("_e") + epsilon_str + string("_l") + localPredRate_str + string("_d") + delta_str
                        + string("_ilen") + base_interval_length_str
                        + string("_rut") + run_up_to_str
                        + string("_run") + to_string(runId);
        
//        ofstream isd_accum_histogram_file(isd_filename);
        ofstream imd_accum_histogram_file(imd_filename);
//        ofstream ismd_accum_histogram_file(ismd_filename);
//        ofstream ismdp_accum_histogram_file(ismdp_filename);
        
        // create accumulative histogram
        cout << " intervalStartDifference histogram: " << endl;        
        isd_accum_histogram = Utility::makeFullAccumHistogram(intervalStartDiffList);
        
        cout << " intervalMinDifference histogram: " << endl;
        imd_accum_histogram = Utility::makeFullAccumHistogram(intervalMinDiffList);
        
        cout << " intervalStartMinDifference histogram: " << endl;
        ismd_accum_histogram = Utility::makeFullAccumHistogram(intervalStartMinDiffList);
        
        cout << " intervalStartMinDifferencePercent histogram: " << endl;
        ismdp_accum_histogram = Utility::makeFullAccumHistogram(intervalStartMinDiffPercentList);
        
        // write histogram on output file
        //Utility::saveFullAccumHistogram(isd_accum_histogram, isd_accum_histogram_file);

        Utility::saveFullAccumHistogram(imd_accum_histogram, imd_accum_histogram_file);

        //Utility::saveFullAccumHistogram(ismd_accum_histogram, ismd_accum_histogram_file);

        //Utility::saveFullAccumHistogram(ismdp_accum_histogram, ismdp_accum_histogram_file);

        // save the false positive rate history	

//        ofstream isd_fpr_file(isd_fpr_filename);
//        ofstream imd_fpr_file(imd_fpr_filename);

	    //Utility::saveFPRH(fHistory_intervalStartDiff, isd_fpr_file);
	    //Utility::saveFPRH(fHistory_intervalMinDiff, imd_fpr_file);

	    // clear false positive rate history
	    Utility::clearFPRH(fHistory_intervalStartDiff);
	    Utility::clearFPRH(fHistory_intervalMinDiff);

        // for debug

        cout << " candidates' generated interval size histogram: " << endl;
        ofstream cand_interval_size_accum_hist_file("./debug/cand_interval_size_accum_hist");
        vector<snapshot_diff_count> cand_interval_size_accum_hist = Utility::makeFullAccumHistogram(candidateIntervalSizeList);
        Utility::saveFullAccumHistogram(cand_interval_size_accum_hist, cand_interval_size_accum_hist_file);
        candidateIntervalSizeList.clear();
        
        cout << " candidates' interval size after split histogram: " << endl;
        ofstream cand_ready_interval_size_accum_hist_file("./debug/cand_ready_interval_size_accum_hist");
        vector<snapshot_diff_count> cand_ready_interval_size_accum_hist = Utility::makeFullAccumHistogram(candidateReadyIntervalSizeList);
        Utility::saveFullAccumHistogram(cand_ready_interval_size_accum_hist, cand_ready_interval_size_accum_hist_file);
        candidateReadyIntervalSizeList.clear();

        // how long to write data onto storage?
        record_time = clock() - record_time;       
        cout << " time to write onto files: ";
        cout.width(15);
        cout << record_time << " clicks      ";
        cout.width(15);
        cout << ((float)record_time)/CLOCKS_PER_SEC << " seconds " << endl;

	    
    }


}

