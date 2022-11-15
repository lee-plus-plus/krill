#ifndef DEFS_H
#define DEFS_H
#include <map>
#include <vector>

// clang-format off
namespace krill {
	namespace type {
		// automata
		struct DFA;
		struct NFA;
		struct Edge;
		using EdgeTable = std::vector<Edge>;
		// grammar
		struct Prod;
		struct Grammar;
		struct Action;
		using ActionTable = std::map<std::pair<int, int>, Action>;
		struct Token;
	} // namespace type

	namespace regex {}
	namespace utils {}
	namespace codegen {}
	namespace runtime {}

} // namespace krill
#endif
// clang-format on