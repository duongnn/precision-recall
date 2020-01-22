/*
  utility.h - class Utility
              for utility functionality
*/

#ifndef UTILITY_H
#define UTILITY_H

#include <iostream> // for cout
#include <vector>
#include <random>

//#define _DEBUG

#define FPRH_NUM    5   // Number of False positive rate histories. Each correspond to one epsilon


using namespace std;

typedef vector<vector<int>> adjacency_matrix;
typedef vector<vector<int>> distance_matrix;
typedef vector<vector<double>> probability_matrix;

extern int epsilon;
extern int number_of_processes;
extern double localPredRate;
extern int delta;
extern double alpha;
extern int run_up_to;
extern string prob_matrix_file_name;
extern int num_of_bins;
extern string topology_file_name;
extern int base_interval_length;

/*
    A structure to help recording false positive rate history over time
*/

class FPRH_Element{ // false positive rate history element
public:
    int totalTrueSoFar;
    int totalSoFar;
    float fpr;  // false positive rate
    
    
    // constructors
    FPRH_Element(){
        totalTrueSoFar = 0;
        totalSoFar = 0;
        fpr = -1;
    }    
    
    FPRH_Element(int tsf, int total, float r){
        totalTrueSoFar = tsf;
        totalSoFar = total;
        fpr = r;
    }    
    
    static float getFPR(int tsf, int total){
        if (total == 0)
            // just return some agreed value
            return 1.5;
        else
            return 1.0 - (1.0*tsf)/(1.0*total);
    }

};


struct _FalsePositiveRateHistory{
    int threshold;  // value of HVC's epsilon to judge whether a cut is true or false positive
//    int totalTrueSoFar; // total number of true positive found so far
    vector<FPRH_Element> history; // history recording number of false positive rate
};

typedef struct _FalsePositiveRateHistory FalsePositiveRateHistory;

extern vector<FalsePositiveRateHistory> fHistory;
extern vector<int> epsilonList;



struct _snapshot_diff_count{
    int value;  // max diff in snapshot
    int count;  // number of such max diff's
    float fraction; // fraction corresponding to count
};

typedef struct _snapshot_diff_count snapshot_diff_count;

/*
  Utility - umbrella class for utility function
*/

class Utility {
public:
    static std::default_random_engine generator;

    static int GetRandomNumberInRange(int, int);
    double Variance(const vector<double>& vi) ;
    static vector<vector<double>> Transpose(const vector<vector<double>>& m_in);
    static std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems) ;
    static std::vector<std::string> split(const std::string &s, char delim);
    static distance_matrix AllPairShortestPaths(const adjacency_matrix& G);
    static adjacency_matrix ReadGraph(const string& topo);
    static probability_matrix ReadProbabilityMatrix(const string& prob);

    // default param can be declared only once. It should be declared in header file
    // thus in utility.cpp, should not provide default value for param size of function
    // GetIndexOfWeightedRandom(). If we provide, it cause compile error or unpredictable runtime
    // http://www.learncpp.com/cpp-tutorial/77-default-parameters/
    static int GetIndexOfWeightedRandom(const vector<double>& arr, int size = number_of_processes);

    static int GetIndexOfWeightedNormalDistribution(double std_percent, int size);

    static vector<snapshot_diff_count> makeFullAccumHistogram(vector<int> data);
    static void saveFullAccumHistogram(vector<snapshot_diff_count> hist, ofstream &os);
    static void saveFPRH(vector<FalsePositiveRateHistory> fh, ofstream &os);
    static void clearFPRH(vector<FalsePositiveRateHistory> fHistory);
    static int GetRandomNumberGeometricDistribution(double p);
};

/*
main(){
  for(int i = 0; i < 10; i++){
    printf("i = %2d rand = %d\n", i, Utility::GetRandomNumberInRange(-5, -2));
  }
}
*/

#endif

