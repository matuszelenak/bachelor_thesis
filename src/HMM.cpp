#include <vector>
#include <fstream>
#include <iostream>
#include <map>

#include "State.h"
#include "LogNum.h"

template < typename val >
using Matrix = std::vector<std::vector<val> >

class HMM{
private:
	int kmer_size;
	std::set<char>bases;
	std::vector<State>states;
	std::vector<vector<LogNum> >transitions;
	Matrix<LogNum>inverse_transitions;
	std::map<std::string, int> kmer_to_state;
	std::vector<std::string> split_string(std::string &s, char delimiter);
	std::vector<std::string> split_string(std::string &s, char delimiter);
	std::vector<std::string> generate_suffixes();

public:
	void loadModelParams(std::string filename);
	void compute_transitions(LogNum prob_skip, LogNum prob_stay);
	std::vector<std::vector<

std::vector<std::string> generate_suffixes(){
	std::vector<std::string>res;
	for (int i = 0; i < bases.size(); i++){
		std::string acc = bases[i];
		for (int j = 0; j < bases.size(); i++){
			acc += bases[j];
			res.push_back(acc);
		}
	}
	return res;
}

std::vector<std::string> split_string(std::string &s, char delimiter){
	std::vector<std::string> res;
	std::string acc = "";
	for (int i = 0; i < s.size(); i++){
		if ((s[i] == delimiter) && (acc.size() > 0)){
			res.push_back(acc);
			acc = "";
		}
		if (s[i] != delimiter) acc += s[i];
	}
	return res;
}

void loadModelParams(std::string filename){
	states.clear();
	State init_state;
	init_state.setParams(0,0,true);
	states.push_back(init_state);

	std::ifstream model_file;
	model_file.open(filename);
	std::string line;
	std::string kmer_label;
	while(getline(model_file, line)) {
		std::vector<std::string> line_elements = split_string(line, '\t');
		kmer_label = line_elements[0];
		double mean = line_elements[1];
		double stdv = line_elements[2];
		//TODO figure out what the remaining two parameters in Nanocall models are for
		State state;
		state.setLabel(kmer_label);
		state.setParams(mean, stdv, false);
		kmer_to_state.insert(pair<std::string, int>(kmer_label, states.size()));
		states.push_back(state);
		for (int i = 0; i < kmer_label.size(); i++){
			bases.insert(kmer_label[i]);
		}
	kmer_size = kmer_label.size();
	}
	model_file.close();
}

void compute_transitions(LogNum prob_skip, LogNum prob_stay){
	transitions.clear();
	transitions.resize(states.size());

	inverse_transitions.clear();
	inverse_transitions.resize(states.size());
	//initialize all probabilites to zero
	for (int i = 0; i < states.size(); i++){
		std::vector<double>t(states.size(), 0);
		transitions.push_back();
		std::vector<double>t2(states.size(), 0);
		inverse_transitions.push_back(t2);
	}
	//initialize transitions from the init state to all others
	for (int i = 1; i < states.size(); i++){
		transitions[0][i] = LogNum(1/(states.size() - 1));
		inverse_transitions[i][0] = LogNum(1/(states.size() - 1));
	}
	//cyclic transitions to states
	for (int i = 1; i < states.size(); i++){
		transitions[i][i] = LogNum(prob_stay);
		inverse_transitions[i][i] = LogNum(prob_stay);
	}
	//transitions that skip base
	std::vector<std::string> suffixes = generate_suffixes();
	for (std::map<std::string, int>::iterator it = kmer_to_state.begin(); it != kmer_to_state.end(); ++it){
		std::string kmer_from = it -> first;
		int state_from = it -> second;
		std::string prefix = kmer_from.substr(2,kmer_size - 2);
		for (int k = 0; k < suffixes.size(); k++){
			std::string kmer_to = prefix + suffixes[k];
			int state_to = kmer_to_state[kmer_to];
			transitions[state_from][state_to] = LogNum(prob_skip);
			inverse_transitions[state_to][state_from] = LogNum(prob_skip);
		}
	}
	//normal transitions
	double normal_trans_prob = (1 - prob_skip - prob_stay) / 4;
	for (std::map<std::string, int>::iterator it = kmer_to_state.begin(); it != kmer_to_state.end(); ++it){
		std::string kmer_from = it -> first;
		int state_from = it -> second;
		std::string prefix = kmer_from.substr(1,kmer_size - 1);
		for (int k = 0; k < bases.size(); k++){
			std::string kmer_to = prefix + bases[k];
			int state_to = kmer_to_state[kmer_to];
			transitions[state_from][state_to] = LogNum(normal_trans_prob);
			inverse_transitions[state_to][state_from] = LogNum(normal_trans_prob);
		}
	}
}

std::vector<int> compute_viterbi_path(std::vector<double> event_sequence){
	Matrix<LogNum>viterbi_matrix;
	Matrix<int> back_ptr;
	for (int i = 0; i < states.size(); i++){
		std::vector<double>row(event_sequence.size(), 0);
		viterbi_matrix.push_back(row);
		std::vector<double>row2(event_sequence.size(), 0);
		back_ptr.push_back(row2);
	}
	viterbi_matrix[0][0] = 1;

	for (int i = 1; i < event_sequence.size(); i++){
		for (int l = 0; l < states.size(); l++){
			LogNum m(0);
			for (int k = 0; k < states.size(); k++){
				if (viterbi_matrix[k][i - 1] + transitions[k][l] > m){
					m = viterbi_matrix[k][i - 1] * transitions[k][l];
					back_ptr[i][l] = k;
				}
			}
			viterbi_matrix[l][i] = m + states[l].get_emission_probability(event_sequence[i]);
		}
	}
	LogNum m(0);
	int last_state = 0;
	for (int k = 0; k < states.size(); k++){
		if (m < viterbi_matrix[k][event_sequence.size() - 1]){
			m = viterbi_matrix[k][event_sequence.size() - 1];
			last_state = k;
		}
	}
	std::vector<int>state_sequence;
	state_sequence.push_back(last_state);
	int prev_state = last_state
	for (int i = event_sequence.size() - 1; i >= 1; i--){
		int s = back_ptr[i][prev_state];
		state_sequence.push_back(s);
		prev_state = s;
	}
	std::reverse(state_sequence.begin(), state_sequence.end());
	return state_sequence;

}

Matrix<LogNum> compute_forward_matrix(std::vector<double> event_sequence){
	Matrix<LogNum>fwd_matrix;
	for (int i = 0; i < states.size(); i++){
		std::vector<double>row(event_sequence.size(), 0);
		fwd_matrix.push_back(row);
	}

	fwd_matrix[0][0] = 1;

	for (int i = 1; i < event_sequence.size(); i++){
		for (int l = 0; l < states.size(); l++){
			LogNum sum(0);
			for (int k = 0; k < states.size(); k++){
				sum += fwd_matrix[k][i - 1] + transitions[k][l];
			}
			fwd_matrix[l][i] = sum + states[l].get_emission_probability(event_sequence[i]);
		}
	}
	return fwd_matrix;
}

std::vector<char> translate_to_bases(std::vector<int>state_sequence){
	
}

Matrix<int> generate_samples()


}