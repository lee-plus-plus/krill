#include "krill/FaModels.h"
#include <iostream>

namespace krill::lexical {

bool Edge::operator<(const Edge &e) const {
    return (symbol < e.symbol ||
            (symbol == e.symbol &&
             (from < e.from || (from == e.from && (to < e.to)))));
}

bool Edge::operator==(const Edge &e) const {
    return (symbol == e.symbol && from == e.from && to == e.to);
}

void DFA::print() {
    std::cout << "hello from DFA" << std::endl;
}


} // namespace krill::lexical