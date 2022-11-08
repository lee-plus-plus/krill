#ifndef UTILS_H
#define UTILS_H
#include "defs.h"
#include "FaModels.h"
#include <ostream>
using std::ostream;

namespace krill::utils {
	void printEdgeTable(krill::lexical::EdgeTable edgeTable, ostream &oss);
	void printDFAasTable(DFA dfa, ostream &oss);
	void printNFAasTable(NFA nfa, ostream &oss);

} // krill::utils
#endif