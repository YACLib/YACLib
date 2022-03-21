#pragma once

#include <string_view>

namespace yaclib {

using LogCallback = void (*)(std::string_view file, std::size_t line, std::string_view function,
                             std::string_view condition, std::string_view message);

#ifdef YACLIB_LOG_ERROR

void SetErrorCallback(LogCallback callback);

namespace detail {
void LogError(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
              std::string_view message);
}  // namespace detail

#  define YACLIB_ERROR(cond, message)                                                                                  \
    do {                                                                                                               \
      if (cond) {                                                                                                      \
        yaclib::detail::LogError(__FILE__, __LINE__, YACLIB_FUNCTION_NAME, (#cond), message);                          \
      }                                                                                                                \
    } while (false)

#else

#  define YACLIB_ERROR(cond, message)                                                                                  \
    do {                                                                                                               \
    } while (false)

inline void SetErrorCallback(LogCallback) {
}

#endif

#ifdef YACLIB_LOG_INFO

void SetInfoCallback(LogCallback callback);

namespace detail {
void LogInfo(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
             std::string_view message);
}  // namespace detail

#  define YACLIB_INFO(cond, message)                                                                                   \
    do {                                                                                                               \
      if (cond) {                                                                                                      \
        yaclib::detail::LogInfo(__FILE__, __LINE__, YACLIB_FUNCTION_NAME, (#cond), message);                           \
      }                                                                                                                \
    } while (false)

#else

#  define YACLIB_INFO(cond, message)                                                                                   \
    do {                                                                                                               \
    } while (false)

inline void SetInfoCallback(LogCallback) {
}

#endif
}  // namespace yaclib

#ifndef YACLIB_FUNCTION_NAME
#  if defined(__clang__) || defined(__GNUC__)
#    define YACLIB_FUNCTION_NAME __PRETTY_FUNCTION__
#  elif defined(_MSC_VER)
#    define YACLIB_FUNCTION_NAME __FUNCSIG__
#  else
#    define YACLIB_FUNCTION_NAME __func__
#  endif
#endif
