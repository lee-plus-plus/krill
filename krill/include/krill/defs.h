#ifndef DEFS_H
#define DEFS_H
#include <map>
#include <vector>

// clang-format off
namespace krill {
	namespace automata {
		struct DFA;
		struct NFA;
		struct Edge;
		using EdgeTable = std::vector<Edge>;
	} // namespace automata
	namespace grammar {
		struct Grammar;
		struct Prod;
		struct ProdItem;
		struct Action;
		using ActionTable = std::map<std::pair<int, int>, Action>;
	} // namespace grammar
	namespace utils {}
	namespace runtime {}

} // namespace krill
#endif
// clang-format on