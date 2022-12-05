#ifndef DEFS_H
#define DEFS_H
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <map>
#include <vector>
#include <iostream>

// clang-format off
namespace krill {
	namespace log {
		extern std::shared_ptr<spdlog::sinks::stdout_color_sink_st> sink_cout;
		extern std::shared_ptr<spdlog::sinks::stderr_color_sink_st> sink_cerr;
		extern std::shared_ptr<spdlog::sinks::basic_file_sink_mt> sink_file;
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