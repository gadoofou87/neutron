#include "assert.hpp"

#ifndef NDEBUG

#include <fmt/color.h>
#include <fmt/core.h>

namespace utils {

namespace detail {

void assert_fail(std::string_view file_name, int line, std::string_view condition,
                 std::string_view message) {
  auto out = fmt::memory_buffer();
  fmt::format_to(std::back_inserter(out), "{}:{}: ASSERT failed due to requirement '{}'", file_name,
                 line, condition);
  if (!message.empty()) {
    fmt::format_to(std::back_inserter(out), " \"{}\"", message);
  }
  fmt::print(stderr, fmt::fg(fmt::terminal_color::red), "{}\n",
             std::string_view(out.data(), out.size()));
  std::abort();
}

}  // namespace detail

}  // namespace utils

#endif
