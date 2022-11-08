#include "krill/defs.h"
#include "krill/automata.h"
#include "Krill/utils.h"
#include <iostream>
using namespace std;
using namespace krill;
using namespace krill::automata;
using namespace krill::automata::utils;
using namespace krill::utils;

// test the transformation from EdgeTable to NFA to DFA
void test1() {
	EdgeTable edgeTable = { // regex: a(b|c+)c
        {'a', 0, 1}, {'b', 1, 2},  {'\0', 1, 3}, // '\0' empty edge
        {'c', 3, 3}, {'\0', 3, 2}, {'b', 2, 4},
    };
    printEdgeTable(edgeTable, cout);

    NFAgraph nfaGraph = toNFAgraph(edgeTable);
	map<int, int> finality = { // must contains all states
		{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 1}
	};
	NFA nfa({nfaGraph, finality});
	printNFAasTable(nfa, cout);

	// should be minimized to: 
	DFA dfa = getDFAfromNFA(nfa);
	printDFAasTable(dfa, cout);
}

// test the minimization of DFA
void test2() {
	DFA dfa = {
		{{0, {{'a', 11}}}, {11, {{'b', 2}}}, 
		 {22, {{'a', 33}}}, {33, {{'b', 0}}}, 
		 {44, {{'a', 55}}}, }, 
		{{0, 0}, {11, 1}, {22, 0}, {33, 1}, {44, 0}, {55, 0}}
	};
	printDFAasTable(dfa, cout);

	DFA dfa2 = getMinimizedDfa(dfa);
	printDFAasTable(dfa2, cout);
}

// test the intergration of multiple DFAs
void test3() {
	DFA dfa1 = { // abcc
		{{0, {{'a', 10}}}, {10, {{'b', 20}}}, 
		 {20, {{'c', 30}}}, {30, {{'c', 0}}},}, 
		{{0, 0}, {10, 1}, {20, 0}, {30, 1},}
	};
	printDFAasTable(dfa1, cout);

	DFA dfa2 = { // a(ba)*
		{{0, {{'a', 1}}}, {1, {{'b', 0}}}, }, 
		{{0, 0}, {1, 1}}
	};
	printDFAasTable(dfa2, cout);

	DFA dfa3 = { // ac(b+c|c)
		{{0, {{'a', 2}}}, {2, {{'c', 4}}}, 
	     {4, {{'b', 5}}}, {5, {{'b', 5}}}, 
	     {5, {{'c', 6}}}, {4, {{'c', 6}}}, }, 
		{{0, 0}, {2, 0}, {4, 0}, {5, 1}, {6, 1}}
	};
	printDFAasTable(dfa3, cout);

	DFA dfai = getDFAintegrated({dfa1, dfa2, dfa3});
	printDFAasTable(dfai, cout);
}

int main() {
	vector<void(*)()> testFuncs = {test1, test2 ,test3};
	for (int i = 0; i < testFuncs.size(); i++) {
		cout << "#test " << i << endl;
		testFuncs[i]();
		cout << endl << endl;
	}
	return 0;
}
