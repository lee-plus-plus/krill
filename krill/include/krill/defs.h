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
		struct Prod;
		struct Grammar;
		struct Action;
		using ActionTable = std::map<std::pair<int, int>, Action>;
		struct Token;
	} // namespace grammar
	namespace utils {}
	namespace codegen {}
	namespace runtime {}

} // namespace krill
#endif
// clang-format on