#ifndef DEFS_H
#define DEFS_H
#include <vector>
#include <map>

namespace krill {
    namespace automata {
        struct DFA;
        struct NFA;
        struct Edge;
        using EdgeTable = std::vector<Edge>;
    }
    namespace grammar {
        struct Grammar;
        struct Prod;
        struct ProdItem;
        struct Action;
        using ActionTable = std::map<std::pair<int, int>, Action>;
    }
    namespace utils {}
    namespace runtime {}

} // namespace compiler
#endif