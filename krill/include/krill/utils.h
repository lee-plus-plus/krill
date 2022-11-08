#ifndef UTILS_H
#define UTILS_H
#include "defs.h"
#include "automata.h"
#include <ostream>
using std::ostream;

namespace krill::utils {
	using krill::automata::DFA, krill::automata::NFA, krill::automata::EdgeTable;

	void printEdgeTable(EdgeTable edgeTable, ostream &oss);
	void printDFAasTable(DFA dfa, ostream &oss);
	void printNFAasTable(NFA nfa, ostream &oss);

} // krill::utils
#endif