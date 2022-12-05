#ifndef DEFS_H
#define DEFS_H
#include <map>
#include <vector>

#include "spdlog/spdlog.h"

// clang-format off
namespace krill {
	namespace log {
		extern spdlog::logger logger;
	}
	namespace type {
		// automata
		struct DFA;
		struct NFA;
		struct Edge;
		using EdgeTable = std::vector<Edge>;
		const int EMPTY_SYMBOL = 0;

		// grammar
		struct Prod;
		struct Grammar;
		struct Action;
		using ActionTable = std::map<std::pair<int, int>, Action>;
		const int END_SYMBOL = -1;

		// lexical
		struct Token;
		extern const Token END_TOKEN;

		// syntax
		struct APTnode;

	} // namespace type

	namespace regex {}
	namespace utils {}
	namespace codegen {}
	namespace runtime {}
	namespace minic {}

} // namespace krill
#endif
// clang-format on