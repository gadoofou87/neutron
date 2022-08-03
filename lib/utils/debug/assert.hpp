#pragma once

#ifdef NDEBUG

#define ASSERT(exp) (void)(exp);

#define ASSERT_X(e0, e1) \
  (void)(e0);            \
  (void)(e1);

#else

#include <string_view>

namespace utils {

namespace detail {

[[noreturn]] void assert_fail(std::string_view file_name, int line, std::string_view condition,
                              std::string_view message);

}  // namespace detail

}  // namespace utils

#define ASSERT(condition)             \
  ((condition) ? static_cast<void>(0) \
               : utils::detail::assert_fail(__FILE_NAME__, __LINE__, #condition, {}))

#define ASSERT_X(condition, message)  \
  ((condition) ? static_cast<void>(0) \
               : utils::detail::assert_fail(__FILE_NAME__, __LINE__, #condition, message))

#endif
