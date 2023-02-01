#include "krill/defs.h"
#include "fmt/format.h"
#include <iostream>
#include <sstream>
#include <string>

template <typename T> inline std::string to_string(const T &v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

namespace krill::log {

#define SPDLOG_TRACE_OFF
#define SPDLOG_DEBUG_ON

std::shared_ptr<spdlog::sinks::stdout_color_sink_st> sink_cout =
    std::make_shared<spdlog::sinks::stdout_color_sink_st>();
std::shared_ptr<spdlog::sinks::stderr_color_sink_st> sink_cerr =
    std::make_shared<spdlog::sinks::stderr_color_sink_st>();
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> sink_file =
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

namespace krill::error {

parse_error::parse_error(int col, int row, const std::string &detail)
    : std::runtime_error(
          to_string(fmt::format("input:{}:{}: error: {}", col, row, detail))),
      col_(col), row_(row), detail_(detail),
      what_arg_(to_string(
          fmt::format("input:{}:{}: error: {}\n", col_, row_, detail_))),
      what_arg_color_(to_string(fmt::format(
          "input:{}:{}: \033[31merror:\033[0m {}\n", col_, row_, detail_))) {
    // write into log
    krill::log::logger.debug(what_arg_);
}

parse_error::~parse_error() throw() {}

const char *parse_error::what() const throw() { return what_arg_color_.c_str(); }

} // namespace krill::error