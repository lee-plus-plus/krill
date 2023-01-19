#include "krill/defs.h"
#include <iostream>

namespace krill::log {

#define SPDLOG_TRACE_OFF
#define SPDLOG_DEBUG_ON

auto sink_cout = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
auto sink_cerr = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
auto sink_file =
    std::make_shared<spdlog::sinks::basic_file_sink_mt>("krill.log");
auto dist_sink = std::make_shared<spdlog::sinks::dist_sink_st>();
spdlog::sinks_init_list sink_list = {sink_cerr, sink_file};
spdlog::logger logger("krill", sink_list.begin(), sink_list.end());

int init_logger_ = []() {
    logger.set_level(spdlog::level::trace); // lowest
    logger.flush_on(spdlog::level::debug);  // flush immediately

    sink_cerr->set_pattern("[%n] [%^%l%$] %v");
    sink_cerr->set_level(spdlog::level::info);
    sink_file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    sink_file->set_level(spdlog::level::debug);

    
    return 0;
}();

} // namespace krill::log