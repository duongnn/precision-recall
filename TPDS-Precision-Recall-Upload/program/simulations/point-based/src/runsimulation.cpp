/*
  runsimulation.cpp - implementation of runsimulation class
*/

#include "runsimulation.h"



int snapshotCount = 0;        // number of snapshot found by Garg's algorithm
set<Token> snapshotAll;   // set of all snapshots true according to Garg's algorithm.
                              // elements/snapshots are sorted according to their maximum time difference
                              // within the snapshot


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
  
    HVC initTs = HVC(vector<int>(number_of_processes, -1*epsilon-1),1);
    Candidate initCand = Candidate(i, 0, initTs);
    
    globalToken.setProcessCandidate(initCand, i);

  }
  
  
  /* initialize false positive history */

  fHistory = vector<FalsePositiveRateHistory>(FPRH_NUM);
  
  for(int i = 0; i < FPRH_NUM; i++){
    fHistory.at(i).threshold = epsilonList.at(i);
    //fHistory.at(i).totalTrueSoFar = 0;
    fHistory.at(i).history = vector<FPRH_Element>(run_up_to, FPRH_Element(0,0, 1.0));
    
  }
  
}


vector<int> RunSimulation::run(int type, const string& s){
    vector<int> maxTimeDiffList; // historgram for maximum time difference
    
    
    // ofstream out(s);                    // output file of HVC size simulation
	//ofstream out_matrix("Matrix_"+s );  // output file of HVC size simulation with more detail
	
    ofstream wcp_out("debug/wcp_out");  // output file containing global snapshot of weak conjunctive predicate

    InitAllConfiguration("parameters.in.linux");
	
	vector<Process> vp = InitProcess();
	
	snapshotCount = 0;

    // clear result
    snapshotAll.clear();


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
            for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++)
                fHistory.at(hist_index).history.at(absolute_time).totalTrueSoFar = fHistory.at(hist_index).history.at(absolute_time-1).totalTrueSoFar;
        }else{
            for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++)
                fHistory.at(hist_index).history.at(absolute_time).totalTrueSoFar = 0;        
        }
        
        
        for_each(begin(vp), end(vp), [&vp, &wcp_out, &maxTimeDiffList](Process& p) {
	
            /* At each time, process will randomly generate candidate,
             i.e. states where local predicates are true.
            */

            p.RandGenCand();

            /* At each cycle clock, process will check to see if it has the global token
             by matching its ID and token's ID
               If match: process the token
               If not match: do nothing
            */

            if(p.GetId() == globalToken.getPid()){
                if(p.ProcessGlobalToken() == WCP_DETECTED){
                    int max_time_diff;
                  
                    // write out the snapshot to output file
                    snapshotCount ++;
                  
                    #ifdef _DEBUG
                  
                    wcp_out << endl << endl << "***** snapshot "<< snapshotCount << ":" << endl;          
                    globalToken.printToken(wcp_out);
                  
                    #endif
                  
                  
                    // record this token
                    // snapshotAll.insert(globalToken);

                    // get maximum time difference within this token
                    max_time_diff = globalToken.getMaxTimeDiff();
                  
                    // save that value for later processing
                    maxTimeDiffList.push_back(max_time_diff);


                    // computing on-the-fly false positive rate for different thresholds
                    for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
                    
                        FPRH_Element* fpr_ptr = &(fHistory.at(hist_index).history.at(absolute_time));
                    
                        if(max_time_diff <= fHistory.at(hist_index).threshold){
                            // one more true positive
                            fpr_ptr->totalTrueSoFar ++;
                            
//                            if(absolute_time == 0)
//                                 fpr_ptr->totalTrueSoFar = 1 is not correct
//                                 since we may have multiple consistent cuts in one absolute_time iteration
//                                fpr_ptr->totalTrueSoFar ++;
//                            else{
//                                if(fpr_ptr->totalTrueSoFar < fHistory.at(hist_index).history.at(absolute_time - 1).totalTrueSoFar)
//                                     first Garg cut found in this absolute_time iteration
//                                    fpr_ptr->totalTrueSoFar = fHistory.at(hist_index).history.at(absolute_time-1).totalTrueSoFar + 1;
//                                else
//                                     next Garg cut found in this absolute_time iteration
//                                    fpr_ptr->totalTrueSoFar ++;
//                            }
                            
                        }else{
                            // one more false positive. True positive unchanged => do nothing
                            
                            
//                            if(absolute_time >=1){
//                                if(fpr_ptr->totalTrueSoFar < fHistory.at(hist_index).history.at(absolute_time - 1).totalTrueSoFar)
//                                    // first Garg cut in this iteration of absolute_time
//                                    fpr_ptr->totalTrueSoFar = fHistory.at(hist_index).history.at(absolute_time-1).totalTrueSoFar;
//                                else{
//                                    // next Garg cut in this iteration of absolute_time
//                                    // do nothing
//                                    // fpr_ptr->totalTrueSoFar  = fpr_ptr->totalTrueSoFar;
//                                }
//                            
//                            }
                        }
                    
                        fpr_ptr->totalSoFar = snapshotCount;
                        fpr_ptr->fpr =  FPRH_Element::getFPR(fpr_ptr->totalTrueSoFar, fpr_ptr->totalSoFar);
                        
                                     
                    }
                  
                    // reset the token/snapshot (all become red) so that we can start looking for new snapshot
                    // globalToken.reset();
                  
                    // in new implementation, we set the process with smallest clock to be RED
                    globalToken.resetSmallest();


                }else{ // not detect any WCP

                    for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
                        FPRH_Element* fpr_ptr = &(fHistory.at(hist_index).history.at(absolute_time));
                        
                        fpr_ptr->totalSoFar = snapshotCount;
                        fpr_ptr->fpr =  FPRH_Element::getFPR(fpr_ptr->totalTrueSoFar, fpr_ptr->totalSoFar);

                    }
                    
//                    if(absolute_time >=1){
//                        // computing on-the-fly false positive rate for different threshold
//                        for(int hist_index = 0; hist_index < FPRH_NUM; hist_index++){
//                            FPRH_Element* fpr_ptr = &(fHistory.at(hist_index).history.at(absolute_time));
//                            FPRH_Element* fpr_ptr_prev = &(fHistory.at(hist_index).history.at(absolute_time - 1));

//                            if(fpr_ptr->totalTrueSoFar < fpr_ptr_prev->totalTrueSoFar){
//                                fpr_ptr->totalTrueSoFar = fpr_ptr_prev->totalTrueSoFar;
//                                fpr_ptr->totalSoFar = fpr_ptr_prev->totalSoFar;
//                                fpr_ptr->fpr = fpr_ptr_prev->fpr;

//                            }else{
//                                // do nothing
//                                // fpr_ptr->totalTrueSoFar  = fpr_ptr->totalTrueSoFar;
//                            }
//                                   
//                        }
//                  
//                    }
                
                }
            }


            #ifdef _DEBUG
            debug_out << "absolute_time = " << absolute_time << " snapshotCount = " << snapshotCount << endl;        
            #endif
            
        });
        

	
	}
	

    #ifdef _DEBUG
    
    cout << "maxTimeDiffList.size() = " << maxTimeDiffList.size() << endl;
    
    #endif
    
    return maxTimeDiffList;
	
  
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

