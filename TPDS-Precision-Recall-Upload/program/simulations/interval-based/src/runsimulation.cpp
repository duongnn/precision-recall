    
//
// TODO:              : what should I compute, store, and how should I display them?
//                      I should display the false positive rate vs. epsilon
//                      However, there are 2 false positive rates:
//                          one false positive rate associated with intervalStartDiff
//                          one false positive rate associated with intervalMinDiff
//                      we should expect the one associated with intervalMinDiff to be smaller
//                      i.e. higher true positive rate (i.e. find more consistent cuts)
//                      => OK
//
//      : implementing 2 vectors
//          => OK
//      : modify Process::RandGenCandidate()
//      : modify Process::sendMsg() and Process::receiveMsg()
// also I should update the method Process:RandGenCandidate in case there is
// a communication at the middle of an interval, how to implement the 2 vectors
//
// also update sendMsg and receiveMsg to split interval, thus making new candidate, thus making
// new snapshot in buffer list and ready list
//

/*
  runsimulation.cpp - implementation of runsimulation class
*/

#include "runsimulation.h"



int snapshotCount = 0;        // number of snapshot found by Garg's algorithm
vector<Token> snapshotAll;   // set of all snapshots true according to Garg's algorithm.
                              // elements/snapshots are sorted according to their maximum time difference
                              // within the snapshot

vector<Token> snapshotBufferList;   // list of snapshots that could be split due to communication
                                    // this list indeed should have at most one element !!!
vector<Token> snapshotReadyList;    // list of snapshots that have expired, thus cannot be split

/* 
  InitiProcess() - initialize all processes with ID starting from 0
*/

vector<Process> RunSimulation::InitProcess() {
	vector<Process> vp;
	for (int i = 0; i < number_of_processes; i++) {
    // Duong: here Process(i) is constructor
		vp.push_back(Process(i));
	}
	return vp;
}


/*
  InitiAllConfiguration() - initialize simulation paramters from input file
                            initialize global variables
*/

void InitAllConfiguration(const string& parameter_file_name) {
    int i;
	ifstream in(parameter_file_name);   // param input file
	string content;

	if (!in.is_open()) {
		printf("Error: can't open parameter file\n");
		exit(1);
	}

	delta_max = delta;
	D = Utility::AllPairShortestPaths(Utility::ReadGraph(topology_file_name));
	prob_matrix = Utility::ReadProbabilityMatrix(prob_matrix_file_name);


    #ifdef _DEBUG

    // init debug output file
    debug_candidate = vector<ofstream>(number_of_processes);
    for(i = 0; i < number_of_processes; i++){
        debug_candidate.at(i).open("debug/wcp_debug_candidate_" + std::to_string(i));
    }

    #endif


    /* initialize global token */

    globalToken.setPid(0);    // process 0 will have global token first

    // mark all candidate as invalid
    globalToken.setColor(vector<int>(number_of_processes, RED));

    // set initial value for each process' candidate
    globalToken.setSnapshot(vector<Candidate>(number_of_processes, Candidate()));
    for(i = 0; i < number_of_processes; i++){
        // initial candidate has Pid = Pid of process, CandId = 0, intervalSize = 1, HVC has all entry
        // less than 0 so that no real HVC happens before this initial HVC
        HVC initTs = HVC(i, vector<int>(number_of_processes, -1*epsilon-1),1);
        Candidate initCand = Candidate(i, 0, initTs, 1);

        globalToken.setProcessCandidate(initCand, i);

    }
    
    
    /* initialize false positive history for 2 types of uncertainty */

    fHistory_intervalStartDiff = vector<FalsePositiveRateHistory>(FPRH_NUM);
    fHistory_intervalMinDiff = vector<FalsePositiveRateHistory>(FPRH_NUM);

    for(int i = 0; i < FPRH_NUM; i++){
        fHistory_intervalStartDiff.at(i).threshold = epsilonList.at(i);
        fHistory_intervalMinDiff.at(i).threshold = epsilonList.at(i);

        fHistory_intervalStartDiff.at(i).history = vector<FPRH_Element>(run_up_to, FPRH_Element(0,0, 1.5));
        fHistory_intervalMinDiff.at(i).history = vector<FPRH_Element>(run_up_to, FPRH_Element(0,0, 1.5));

    }
  
}

////
//// insert a consistent snapshot/global token into the buffer list
////
//void insertToBufferList(globalToken){

//    snapshotBufferList.push_back(globalToken);

//}

////
//// check if a token has expired
////  i.e. whether its last interval has passed a certain time
////

//bool hasExpired(Token t, int expire_time){
//    t.updateEndpoints();
//    
//    if (t.getLastEnd() <= expire_time)
//        return false;
//    else
//        return true;
//}

////
//// checkBufferList(): local function of runsimulation
////                  scan snapshots within the buffer list to find snapshot whose endpoints of all
////                  intervals have passed current value of absolute_time
//                  
//void checkBufferList(int expire_time){
//    vector<Token> it = snapshotBufferList.begin();

//    // scan buffer list to find any snapshot which has expired, then move to ready list    
//    while(it != snapshotBufferList.end()){
//        if(hasExpired(*it, expire_time)){
//            Token a_token = *it;
//            
//            // move to ready list
//            snapshotReadyList.push_back(a_token);
//            
//            // erase and change to next
//            it = snapshotBufferList.erase(it);
//        }else{
//            it++;
//        }
//    }
//}

////
//// cleanReadyList(): analyze data in ready list
////

//void cleanReadyList(ofstream& os, int abs_time){

//    vector<Token> it = snapshotReadyList.begin();
//    
//    if(snapshotReadyList.empty())
//        return;
//    
//    while(it != snapshotReadyList.end()){
//        Token token = *it;
//        int interval_start_diff, interval_min_diff;
//      
//        // write out the snapshot to output file
//        snapshotCount ++;
//      
//        #ifdef _DEBUG
//      
//        os << endl << endl << "***** snapshot "<< snapshotCount << ":" << endl;
//        token.printToken(os);
//      
//        #endif
//      
//        // get intervals' start difference
//        token.updateEndPoints();
//        interval_start_diff = token.getIntervalStartDiff();
//        interval_min_diff = token.getIntervalMinDiff();
//        
//        // save the values
//        intervalStartDiffList.push_back(interval_start_diff);
//        intervalMinDiffList.push_back(interval_min_diff);


//        // computing on-the-fly false positive rate for different thresholds
//        for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){

//            FPRH_Element* fpr_ptr_isd = &(fHistory_intervalStartDiff.at(hist_index).history.at(abs_time));
//            FPRH_Element* fpr_ptr_imd = &(fHistory_intervalMinDiff.at(hist_index).history.at(abs_time));
//            
//            if(interval_start_diff <= fHistory_intervalStartDiff.at(hist_index).threshold){
//                // one more true positive
//                fpr_ptr_isd->totalTrueSoFar ++;
//            }else{
//                // one more false positive, do nothing
//            }
//            
//            if(interval_min_diff <= fHistory_interavalMinDiff.at(hist_index).threshold)){
//                // one more true positive
//                fpr_ptr_imd->totalTrueSoFar ++;
//            }else{
//                // one more false positive, do thing
//            }
//            
//            fpr_ptr_isd->totalSoFar = snapshotCount;
//            fpr_ptr_isd->fpr =  FPRH_Element::getFPR(fpr_ptr_isd->totalTrueSoFar, fpr_ptr_isd->totalSoFar);
//            fpr_ptr_imd->totalSoFar = snapshotCount;
//            fpr_ptr_imd->fpr =  FPRH_Element::getFPR(fpr_ptr_imd->totalTrueSoFar, fpr_ptr_imd->totalSoFar);
//                         
//        }

//        // move to next element    
//        it ++;
//        
//    } // end while loop (it)

//    // now all elements in snapshotReadyList have been considered, we need to empty it
//    snapshotReadyList.clear();

//}


void RunSimulation::run(int type, const string& simulationStr){
    
    ofstream wcp_out("debug/wcp_out");  // output file containing global snapshot of weak conjunctive predicate

    InitAllConfiguration("parameters.in.linux");
	
    vector<Process> vp = InitProcess();
	
    snapshotCount = 0;

    // clear result for new run
    snapshotAll.clear();
    
    intervalStartDiffList.clear();
    intervalMinDiffList.clear();
    intervalStartMinDiffList.clear();
    intervalStartMinDiffPercentList.clear();
    snapshotBufferList.clear();
    snapshotReadyList.clear();


    /* simulation main loop */
 
	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
		
		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
		
		for_each(begin(vp), end(vp), [&vp, &type](Process& p) {
		    // sending message according to desired distribution
		    switch(type){
		        case RAND_UNICAST:			
    				p.SendMsg(vp[Utility::GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
    				break;
    				
    		    //
    			// cases for other sending patterns
    			//
    			
    		    default:
    		        break;
    		}
		});


        /* for Garg's algorithm */
        
        // inherit the number of true positive found in previous iteration of absolute_time
        if (absolute_time >=1){
            for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
                fHistory_intervalStartDiff.at(hist_index).history.at(absolute_time).totalTrueSoFar = 
                    fHistory_intervalStartDiff.at(hist_index).history.at(absolute_time-1).totalTrueSoFar;
                fHistory_intervalMinDiff.at(hist_index).history.at(absolute_time).totalTrueSoFar = 
                    fHistory_intervalMinDiff.at(hist_index).history.at(absolute_time-1).totalTrueSoFar;
            }
        }else{
            for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
                fHistory_intervalStartDiff.at(hist_index).history.at(absolute_time).totalTrueSoFar = 0;
                fHistory_intervalMinDiff.at(hist_index).history.at(absolute_time).totalTrueSoFar = 0;
            }
        }
        
        
        //for_each(begin(vp), end(vp), [&vp, &wcp_out, &intervalStartDiffList, &intervalMinDiffList](Process& p) {
        for_each(begin(vp), end(vp), [&vp, &wcp_out](Process& p) {
	
	        // check if the candidates in candidateBuffer has expired, if yes move it to candidateQueue
	        // note: checkCandidateBuffer should set firstflag to true
	        	        
	        p.checkCandidateBuffer();

	
            //  generate local predicate randomly if there is no active candidate

            p.RandGenCand();
	        
            // Process the token if it has that responsibility.
            // At each cycle clock, process will check to see if it has the global token
            // by matching its ID and token's ID
            //   If match: process the token
            //   If not match: do nothing

            if(p.GetId() == globalToken.getPid()){
                if(p.ProcessGlobalToken() == WCP_DETECTED){
                
                    // Note: all candidates proposed for consideration as element of token are guaranteed
                    // to have expired, thus will not be split. Thus we just process as normal,
                    // not need to place the token into buffer list.

                    int interval_start_diff, interval_min_diff;
                  
                    // count this snapshot
                    snapshotCount ++;

                    // write out the snapshot to output file for debugging
                    #ifdef _DEBUG
                  
                    wcp_out << endl << endl << "***** snapshot "<< snapshotCount << ":" << endl;          
                    globalToken.printToken(wcp_out);
                  
                    #endif
                  
                  
                    // for now, we do not need to record this token
                    // snapshotAll.push_back(globalToken);

                    // get intervals' start difference
                    globalToken.updateEndPoints();
                    interval_start_diff = globalToken.getIntervalStartDiff();
                    interval_min_diff = globalToken.getIntervalMinDiff();
                    
                    // save the values
                    intervalStartDiffList.push_back(interval_start_diff);
                    intervalMinDiffList.push_back(interval_min_diff);
                    intervalStartMinDiffList.push_back(interval_start_diff - interval_min_diff);
                    if(interval_start_diff == 0){
                        intervalStartMinDiffPercentList.push_back(0);
                    }else{
                        intervalStartMinDiffPercentList.push_back( (100*(interval_start_diff - interval_min_diff))/interval_start_diff);
                    }


                    // computing on-the-fly false positive rate for different thresholds
                    for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){

                        FPRH_Element* fpr_ptr_isd = &(fHistory_intervalStartDiff.at(hist_index).history.at(absolute_time));
                        FPRH_Element* fpr_ptr_imd = &(fHistory_intervalMinDiff.at(hist_index).history.at(absolute_time));
                        
                        if(interval_start_diff <= fHistory_intervalStartDiff.at(hist_index).threshold){
                            // one more true positive
                            fpr_ptr_isd->totalTrueSoFar ++;
                        }else{
                            // one more false positive, do nothing
                        }
                        
                        if(interval_min_diff <= fHistory_intervalMinDiff.at(hist_index).threshold){
                            // one more true positive
                            fpr_ptr_imd->totalTrueSoFar ++;
                        }else{
                            // one more false positive, do thing
                        }

                        fpr_ptr_isd->totalSoFar = snapshotCount;
                        fpr_ptr_isd->fpr =  FPRH_Element::getFPR(fpr_ptr_isd->totalTrueSoFar, fpr_ptr_isd->totalSoFar);
                        fpr_ptr_imd->totalSoFar = snapshotCount;
                        fpr_ptr_imd->fpr =  FPRH_Element::getFPR(fpr_ptr_imd->totalTrueSoFar, fpr_ptr_imd->totalSoFar);
                                     
                    }
                  

                    // reset the global token so that we can looking for new snapshot
                    // we set the process with smallest clock to be RED
                    globalToken.resetSmallest();


                }else{ // not detect any WCP

                    for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
                        FPRH_Element* fpr_ptr_isd = &(fHistory_intervalStartDiff.at(hist_index).history.at(absolute_time));
                        FPRH_Element* fpr_ptr_imd = &(fHistory_intervalMinDiff.at(hist_index).history.at(absolute_time));
                        
                        fpr_ptr_isd->totalSoFar = snapshotCount;
                        fpr_ptr_isd->fpr =  FPRH_Element::getFPR(fpr_ptr_isd->totalTrueSoFar, fpr_ptr_isd->totalSoFar);
                        fpr_ptr_imd->totalSoFar = snapshotCount;
                        fpr_ptr_imd->fpr =  FPRH_Element::getFPR(fpr_ptr_imd->totalTrueSoFar, fpr_ptr_imd->totalSoFar);

                    }
                    
                    
                
                }
            } // end if (p.GetId() == globalToken.getPid())


            #ifdef _DEBUG
            debug_out << "absolute_time = " << absolute_time << " snapshotCount = " << snapshotCount << endl;        
            #endif
            
        });
        	
	} // end for loop (absolute_time = 0; ...)
	

    #ifdef _DEBUG
    
    cout << "intervalStartDiffList.size() = " << intervalStartDiffList.size() 
        << " intervalMinDiffList.size() = " << intervalMinDiffList.size() 
        << " snapshotCount = " << snapshotCount << endl;
    
    #endif
    	
  
}





/*
  // belows are code from Sorrachai that has not been used

double Variance(const vector<double>& vi) {
	double avg = accumulate(begin(vi), end(vi), 0.0) / vi.size();
	double ss = 0;
	for (auto x : vi) ss += (x - avg)*(x-avg);
	return ss / vi.size();
}

vector<vector<double>> Transpose(const vector<vector<double>>& m_in) {
	if (m_in.size() == 0) {
		printf("Warning Transpose empty matrix\n");
		return vector<vector<double>>();
	}
	int m = m_in[0].size();
	int n = m_in.size();
	vector<vector<double>> m_out(m, vector<double>(n, 0));
	for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
			m_out[j][i] = m_in[i][j];
	return m_out;
}


double inline RandomUnicastExperimentWithNormalDistribution(double percentstd) {
	vector<Process> vp = InitProcess();
	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
    // Duong: what this line does? what does for_each do?
    // lambda expression
    // [var outside scope](param){statements}
    // [] means? nothing outside
    // [&] means? everythings outside
    // [&vp] means? just vp outside

//   for(auto & p: vp) {
//      p.UnlockTime();
//      p.ReceiveMsg();
//    }
  
		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });

  
//   for(auto & p: vp) {
//      p.SendMsg(vp[GetIndexOfWeightedNormalDistribution(percentstd)]);
//    }
  

		for_each(begin(vp), end(vp), [&](Process& p) {
			//	rintf("random : %f\n", PhysicalTime::GetRandomNumberInRange(1, 100)*1.0);
			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
			p.SendMsg(vp[GetIndexOfWeightedNormalDistribution(percentstd)]);
			//}
		});
	}

	double num_events = 0;
	double sum_size_active_all = 0;
	for_each(begin(vp), end(vp), [&](const Process& p) { num_events += p.GetNumEvents(); sum_size_active_all += p.GetSumSizeActive(); });

	//cout << "SUM CLOCK SIZE is " << sum_size_active_all << endl;
	//cout << "NUM EVENTS is " << num_events << endl;

	//cout << "AVERAGE CLOCK SIZE is " << sum_size_active_all / num_events << endl;
	cout << "AVERAGE CLOCK SIZE/n " << sum_size_active_all / (num_events *number_of_processes) << endl;

	return sum_size_active_all / (num_events *number_of_processes);
}

double inline RandomUnicastExperiment() {
	vector<Process> vp = InitProcess();
	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
		for_each(begin(vp), end(vp), [&vp](Process& p) {
	//	rintf("random : %f\n", PhysicalTime::GetRandomNumberInRange(1, 100)*1.0);
			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
				p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
			//}
		});
	}

	double num_events = 0;
	double sum_size_active_all = 0;
	for_each(begin(vp), end(vp), [&](const Process& p) { num_events += p.GetNumEvents(); sum_size_active_all += p.GetSumSizeActive(); });

	//cout << "SUM CLOCK SIZE is " << sum_size_active_all << endl;
	//cout << "NUM EVENTS is " << num_events << endl;

	//cout << "AVERAGE CLOCK SIZE is " << sum_size_active_all / num_events << endl;
	cout << "AVERAGE CLOCK SIZE/n " << sum_size_active_all / (num_events *number_of_processes) << endl;

	return sum_size_active_all / (num_events *number_of_processes);
}


void inline RandomUnicastExperimentSnapshotVariance(const string& s) {
	ofstream out(s);
	vector<Process> vp = InitProcess();
	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
		for_each(begin(vp), end(vp), [&vp](Process& p) {

			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
			p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
			//}
		});
		double sum_size_active_all = 0;
		vector<double> current_size_distribution;
		for_each(begin(vp), end(vp), [&](Process& p) {int  casn = p.CountActiveSizeNow(); current_size_distribution.push_back(casn / (float)number_of_processes); });
		out << Variance(current_size_distribution) << endl;
	}


}
void inline RandomUnicastExperimentSnapshotY2(const string& s) {
	ofstream out(s);
	vector<Process> vp = InitProcess();
	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
		for_each(begin(vp), end(vp), [&vp](Process& p) {

			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
			p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
			//}
		});
		double sum_sq_size_active_all = 0;
		vector<double> current_size_distribution;
		for_each(begin(vp), end(vp), [&](Process& p) {int  casn = p.CountActiveSizeNow();  sum_sq_size_active_all += casn*casn;  });
		//	out_matrix << endl;
		out << sum_sq_size_active_all / (vp.size()*vp.size()) << endl;
	}


}



void inline VaryEpsilon(const string& in, int epsupto) {
//string s = in + to_string(alpha*100)  string(".txt");

	string s = in +  string(".txt");
	ofstream out(s);
	
	for (int i = 1; i <= epsupto; i++) {
		printf("epsilon %d ", i);
		epsilon = i;
		out << RandomUnicastExperiment() << endl;
	}
}
void inline VaryEpsilonWithNormalDistribution(double stdpercent) {
//string s = string("normaldist-delta10alpha25") + to_string(alpha*100)  string(".txt");
	
	string s = string("normaldist-delta10alpha25") +   string(".txt");
	ofstream out(s);
	
	for (int i = 1; i <= 150; i++) {
		printf("epsilon %d ", i);
		epsilon = i;
		out << RandomUnicastExperimentWithNormalDistribution(stdpercent) << endl;
	}
}

void inline VaryDelta(const vector<int>& deltas, int eps_up_to) {
	vector<vector<double>> charts;
	for (int d : deltas) {
		delta =delta_max= d;
		printf("delta=%d\n", d);
		vector<double> plot_by_eps;
		for (int i = 1; i <= eps_up_to; i+=4) {
			epsilon = i;
			plot_by_eps.push_back(RandomUnicastExperiment());
		} 
		charts.push_back(plot_by_eps);
		//cout << endl;
	}
	ofstream out("n=100,alpha=1,vdelta100.txt");
	charts = Transpose(charts);
	for (auto& row : charts) {
		for (double v : row) {
			out << v << " ";
		}
		out << endl;
	}
}

void inline VaryAlpha(const vector<double>& alphas, int eps_up_to) {
	vector<vector<double>> charts;
	for (double a : alphas) {
		printf("alpha=%f\n", a);
		alpha = a;
		vector<double> plot_by_eps;
		for (int i = 1; i <= eps_up_to; i += 4) {
			epsilon = i;
			plot_by_eps.push_back(RandomUnicastExperiment());
		}
		charts.push_back(plot_by_eps);
	}
	ofstream out("n=100,delta=30,varyalpha,+4to800.txt");
	charts = Transpose(charts);
	for (auto& row : charts) {
		for (double v : row) {
			out << v << " ";
		}
		out << endl;
	}
}

*/

