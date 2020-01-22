// /*
//   wcp.cpp - old main program
// */

// #include <vector>
// #include <string>
// #include <sstream>
// #include"Process.h"

// using namespace std;

// #define INF 1000000	# infinity


// //Define and assigned global variable in main.cpp
// int delta = 1;
// int epsilon = 10;
// int delta_max = delta;
// double alpha = 0.5;
// int run_up_to = 100000;
// int number_of_processes = 10;
// distance_matrix D;
// probability_matrix prob_matrix;
// string topology_file_name;
// string prob_matrix_file_name;
// int absolute_time;

// // For Garg's algorithm
// Token globalToken;
// double localPredRate = 0.01;    // randomly generate localPredicate according to this parameter


// // for debug
// ofstream debug_out("wcp_debug_out");
// vector<ofstream> debug_candidate;


// // Duong: why use inline function? maybe for speed
// vector<Process> inline InitProcess() {
// 	vector<Process> vp;
// 	for (int i = 0; i < number_of_processes; i++) {
//     // Duong: here Process(i) is constructor
// 		vp.push_back(Process(i));
// 	}
// 	return vp;
// }

// void InitAllConfiguration(const string& parameter_file_name) {
//   int i;

// 	ifstream in(parameter_file_name);
// 	if (!in.is_open()) {
// 		printf("Error: can't open parameter file\n");
// 		exit(1);
// 	}
// 	string content;
// 	while (getline(in, content)) {
// 		vector<string> split_equalsign = split(content, '=');
// 		for (string& s : split_equalsign) {
// 			cout << s << "  ";
// 		}
// 		printf("\n");
// 		string s0 = split_equalsign[0];
// 		string s1 = split_equalsign[1];

//                                                                                     //    cout << "split_equalsign[0] = " << split_equalsign[0] ;
// //    cout << "split_equalsign[1] = " << split_equalsign[1] ;


// 		if (s0 == "delta") {
// 			delta = atoi(s1.c_str());
// 		}
// 		else if (s0 == "epsilon") {
// 			epsilon = atoi(s1.c_str());
// 		}
// 		else if (s0 == "alpha") {
// 			alpha = atof(s1.c_str());
// 		}
// 		else if (s0 == "run_up_to") {
// 			run_up_to = atoi(s1.c_str());
// 		}
// 		else if (s0 == "number_of_processes") {
// 			number_of_processes = atoi(s1.c_str());
// 		} 
// 		else if (s0 == "topology_file") {
// 			topology_file_name = s1;

//       cout << "get topology_file" << " is " << s1 << " ";

// 		}
// 		else if (s0 == "probability_matrix_file") {
// 			prob_matrix_file_name = s1;
// 		}
// 		else { printf("else;\n"); }
// 	}
// 	delta_max = delta;
// 	D = AllPairShortestPaths(ReadGraph(topology_file_name));
// 	prob_matrix = ReadProbabilityMatrix(prob_matrix_file_name);

//   // init debug output file

//   //debug_candidate.resize(number_of_processes); // ===> error
//   debug_candidate = vector<ofstream>(number_of_processes);

//   for(i = 0; i < number_of_processes; i++){
//     debug_candidate.at(i).open("wcp_debug_candidate_" + std::to_string(i));
//   }


//   /* initialize global token */

//   globalToken.pid = 0;    // process 0 will have global token first

//   //globalToken.snapshot.resize(number_of_processes);
//   //globalToken.color.resize(number_of_processes);
//   globalToken.snapshot = vector<Candidate>(number_of_processes, Candidate());
//   globalToken.color = vector<int>(number_of_processes, RED);

//   for(i = 0; i < number_of_processes; i++){
//     // snapshot has initial/default candidates

//     //snapshot.at(i) = candidate();

//     globalToken.snapshot.at(i).procId = i;
//     globalToken.snapshot.at(i).candId = 0;
//     globalToken.snapshot.at(i).timestamp = HVC(vector<int>(number_of_processes, -1*epsilon-1),1);

//     // all candidates are invalid
//     // color.at(i) = RED;
//   }

// }

// double inline RandomUnicastExperimentWithNormalDistribution(double percentstd) {
// 	vector<Process> vp = InitProcess();
// 	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
//     // Duong: what this line does? what does for_each do?
//     // lambda expression
//     // [var outside scope](param){statements}
//     // [] means? nothing outside
//     // [&] means? everythings outside
//     // [&vp] means? just vp outside
// /*
//      for(auto & p: vp) {
//         p.UnlockTime();
//         p.ReceiveMsg();
//       }
// */
// 		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });

// /*
//      for(auto & p: vp) {
//         p.SendMsg(vp[GetIndexOfWeightedNormalDistribution(percentstd)]);
//       }
// */

// 		for_each(begin(vp), end(vp), [&](Process& p) {
// 			//	rintf("random : %f\n", PhysicalTime::GetRandomNumberInRange(1, 100)*1.0);
// 			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
// 			p.SendMsg(vp[GetIndexOfWeightedNormalDistribution(percentstd)]);
// 			//}
// 		});
// 	}

// 	double num_events = 0;
// 	double sum_size_active_all = 0;
// 	for_each(begin(vp), end(vp), [&](const Process& p) { num_events += p.GetNumEvents(); sum_size_active_all += p.GetSumSizeActive(); });

// 	//cout << "SUM CLOCK SIZE is " << sum_size_active_all << endl;
// 	//cout << "NUM EVENTS is " << num_events << endl;

// 	//cout << "AVERAGE CLOCK SIZE is " << sum_size_active_all / num_events << endl;
// 	cout << "AVERAGE CLOCK SIZE/n " << sum_size_active_all / (num_events *number_of_processes) << endl;

// 	return sum_size_active_all / (num_events *number_of_processes);
// }

// double inline RandomUnicastExperiment() {
// 	vector<Process> vp = InitProcess();
// 	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
// 		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
// 		for_each(begin(vp), end(vp), [&vp](Process& p) {
// 	//	rintf("random : %f\n", PhysicalTime::GetRandomNumberInRange(1, 100)*1.0);
// 			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
// 				p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
// 			//}
// 		});
// 	}

// 	double num_events = 0;
// 	double sum_size_active_all = 0;
// 	for_each(begin(vp), end(vp), [&](const Process& p) { num_events += p.GetNumEvents(); sum_size_active_all += p.GetSumSizeActive(); });

// 	//cout << "SUM CLOCK SIZE is " << sum_size_active_all << endl;
// 	//cout << "NUM EVENTS is " << num_events << endl;

// 	//cout << "AVERAGE CLOCK SIZE is " << sum_size_active_all / num_events << endl;
// 	cout << "AVERAGE CLOCK SIZE/n " << sum_size_active_all / (num_events *number_of_processes) << endl;

// 	return sum_size_active_all / (num_events *number_of_processes);
// }

// void inline RandomUnicastExperimentSnapshot(const string& s) {
// 	//string s = string("Simulation1-delta10alpha0.25ep30") + /*to_string(alpha*100)*/  string(".txt");
// 	ofstream out(s);
// 	ofstream out_matrix("Matrix_"+s );

//   // output file containing global snapshot of weak conjunctive predicate
//   ofstream wcp_out("wcp_out");

// 	vector<Process> vp = InitProcess();

//   // program main loop here
 
// 	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
// 		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
// 		for_each(begin(vp), end(vp), [&vp](Process& p) {
			
// 			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
// 				p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
// 			//}
// 		});


//     /* Additional functionality:
//         Randomly generate true local predicate
//         Process Token if it is its turn
//     */

// 		for_each(begin(vp), end(vp), [&vp, &wcp_out](Process& p) {
			
//       /* At each time, process will randomly generate candidate,
//          i.e. states where local predicates are true.
//       */
      
//       p.RandGenCand();

//       /* At each time, process will check to see if it has token
//          If it does not have token, then it will ignore
//          If it has the token, it will process the token
//           + take next candidate until it become green
//             update color for other process accordingly
//             and change value of token accordingly (to pass token to other process)
//           + if candidate runs out, it just return, but doesn't change value of token
//             so that it will be able to process the token in the next run
//       */

//       if(p.GetId() == globalToken.pid)
//         if(p.ProcessGlobalToken() == WCP_DETECTED){
//           // write out the snapshot to output file
//           globalToken.printToken(wcp_out);

//           cout << "***** SUCCESS: global predicate found" << endl;

//           // if we do not have to find all global snapshot, we can terminate here
//           exit(EXIT_SUCCESS);


//         }

// 		});
    

// 		double sum_size_active_all = 0;
// 		vector<double> current_size_distribution;
// 		for_each(begin(vp), end(vp), [&](Process& p) {int  casn = p.CountActiveSizeNow();  sum_size_active_all += casn; out_matrix << casn / (float)number_of_processes << " "; });
// 		out_matrix << endl;
// 		out << sum_size_active_all / (vp.size()*vp.size()) << endl;
// 	}
// }

// void inline RandomUnicastExperimentSnapshotVariance(const string& s) {
// 	ofstream out(s);
// 	vector<Process> vp = InitProcess();
// 	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
// 		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
// 		for_each(begin(vp), end(vp), [&vp](Process& p) {

// 			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
// 			p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
// 			//}
// 		});
// 		double sum_size_active_all = 0;
// 		vector<double> current_size_distribution;
// 		for_each(begin(vp), end(vp), [&](Process& p) {int  casn = p.CountActiveSizeNow(); current_size_distribution.push_back(casn / (float)number_of_processes); });
// 		out << Variance(current_size_distribution) << endl;
// 	}


// }
// void inline RandomUnicastExperimentSnapshotY2(const string& s) {
// 	ofstream out(s);
// 	vector<Process> vp = InitProcess();
// 	for (absolute_time = 0; absolute_time < run_up_to; absolute_time++) {
// 		for_each(begin(vp), end(vp), [](Process& p) {p.UnlockTime(); p.ReceiveMsg(); });
// 		for_each(begin(vp), end(vp), [&vp](Process& p) {

// 			//if (PhysicalTime::GetRandomNumberInRange(1, 100)*1.0 <= alpha * 100) {
// 			p.SendMsg(vp[GetIndexOfWeightedRandom(prob_matrix[p.GetId()])]);
// 			//}
// 		});
// 		double sum_sq_size_active_all = 0;
// 		vector<double> current_size_distribution;
// 		for_each(begin(vp), end(vp), [&](Process& p) {int  casn = p.CountActiveSizeNow();  sum_sq_size_active_all += casn*casn;  });
// 		//	out_matrix << endl;
// 		out << sum_sq_size_active_all / (vp.size()*vp.size()) << endl;
// 	}


// }
// void inline VaryEpsilon(const string& in, int epsupto) {
// 	string s = in + /*to_string(alpha*100)*/  string(".txt");
// 	ofstream out(s);
// 	for (int i = 1; i <= epsupto; i++) {
// 		printf("epsilon %d ", i);
// 		epsilon = i;
// 		out << RandomUnicastExperiment() << endl;
// 	}
// }
// void inline VaryEpsilonWithNormalDistribution(double stdpercent) {
// 	string s = string("normaldist-delta10alpha25") + /*to_string(alpha*100)*/  string(".txt");
// 	ofstream out(s);
// 	for (int i = 1; i <= 150; i++) {
// 		printf("epsilon %d ", i);
// 		epsilon = i;
// 		out << RandomUnicastExperimentWithNormalDistribution(stdpercent) << endl;
// 	}
// }
// void inline VaryDelta(const vector<int>& deltas, int eps_up_to) {
// 	vector<vector<double>> charts;
// 	for (int d : deltas) {
// 		delta =delta_max= d;
// 		printf("delta=%d\n", d);
// 		vector<double> plot_by_eps;
// 		for (int i = 1; i <= eps_up_to; i+=4) {
// 			epsilon = i;
// 			plot_by_eps.push_back(RandomUnicastExperiment());
// 		} 
// 		charts.push_back(plot_by_eps);
// 		//cout << endl;
// 	}
// 	ofstream out("n=100,alpha=1,vdelta100.txt");
// 	charts = Transpose(charts);
// 	for (auto& row : charts) {
// 		for (double v : row) {
// 			out << v << " ";
// 		}
// 		out << endl;
// 	}
// }
// void inline VaryAlpha(const vector<double>& alphas, int eps_up_to) {
// 	vector<vector<double>> charts;
// 	for (double a : alphas) {
// 		printf("alpha=%f\n", a);
// 		alpha = a;
// 		vector<double> plot_by_eps;
// 		for (int i = 1; i <= eps_up_to; i += 4) {
// 			epsilon = i;
// 			plot_by_eps.push_back(RandomUnicastExperiment());
// 		}
// 		charts.push_back(plot_by_eps);
// 	}
// 	ofstream out("n=100,delta=30,varyalpha,+4to800.txt");
// 	charts = Transpose(charts);
// 	for (auto& row : charts) {
// 		for (double v : row) {
// 			out << v << " ";
// 		}
// 		out << endl;
// 	}
// }

// int main() {

// 	InitAllConfiguration("parameters.in.linux");
// 	RandomUnicastExperimentSnapshot("Delay-n1000,run1000,alpha0d1,eps100,delta1");

//   // Duong: after this main loop, all processes have terminated and no global predicate found
//   cout << "***** FAILURE: No global predicate found " << endl;
	
// 	return EXIT_FAILURE;
// }
