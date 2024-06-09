#pragma once

#include <string_view>

namespace yaclib {

// TODO(MBkkt) Maybe add callbacks overload for source_location, stacktrace arguments?

using LogCallback = void (*)(std::string_view file, std::size_t line, std::string_view function,
                             std::string_view condition, std::string_view message) noexcept;

namespace detail {

enum class LogLevel : unsigned char {
  Debug = 0,
  Warn = 1,
  Count = 3,
};

void SetCallback(LogLevel level, LogCallback callback) noexcept;

void LogMessage(LogLevel level, std::string_view file, std::size_t line, std::string_view func,
                std::string_view condition, std::string_view message) noexcept;

}  // namespace detail
}  // namespace yaclib

#ifndef YACLIB_FUNC_NAME
#  if defined(__clang__) || defined(__GNUC__)
#    define YACLIB_FUNC_NAME __PRETTY_FUNCTION__
#  elif defined(_MSC_VER)
#    define YACLIB_FUNC_NAME __FUNCSIG__
#  else
#    define YACLIB_FUNC_NAME __func__
#  endif
#endif

#define YACLIB_LOG_MESSAGE(level, cond, message)                                                                       \
  do {                                                                                                                 \
    if (!!(cond)) {                                                                                                    \
      ::yaclib::detail::LogMessage(level, __FILE__, __LINE__, YACLIB_FUNC_NAME, #cond, (message));                     \
    }                                                                                                                  \
  } while (false)

#define YACLIB_SET_CALLBACK(level, callback)                                                                           \
  do {                                                                                                                 \
    ::yaclib::detail::SetCallback(level, (callback));                                                                  \
  } while (false)

#ifdef NDEBUG
#  define YACLIB_STUB1(first) ((void)1)
#  define YACLIB_STUB2(first, second) ((void)1)
#else
#  define YACLIB_STUB1(first)                                                                                          \
    do {                                                                                                               \
      if (false) {                                                                                                     \
        (void)(first);                                                                                                 \
      }                                                                                                                \
    } while (false)
#  define YACLIB_STUB2(first, second)                                                                                  \
    do {                                                                                                               \
      if (false) {                                                                                                     \
        (void)(first);                                                                                                 \
        (void)(second);                                                                                                \
      }                                                                                                                \
    } while (false)
#endif

#ifdef YACLIB_LOG_WARN
#  define YACLIB_INIT_WARN(callback) YACLIB_SET_CALLBACK(::yaclib::detail::LogLevel::Warn, callback)
#  define YACLIB_WARN(cond, message) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Warn, cond, message)
#else
#  define YACLIB_INIT_WARN(callback) YACLIB_STUB1(callback)
#  define YACLIB_WARN(cond, message) YACLIB_STUB2(cond, message)
#endif

#ifdef YACLIB_LOG_DEBUG
#  define YACLIB_INIT_DEBUG(callback) YACLIB_SET_CALLBACK(::yaclib::detail::LogLevel::Debug, callback)
#  define YACLIB_DEBUG(cond, message) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Debug, cond, message)
#  define YACLIB_ASSERT(cond) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Debug, !(cond), "")
#  define YACLIB_PURE_VIRTUAL() YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Debug, true, "Pure virtual call!")
#else
#  define YACLIB_INIT_DEBUG(callback) YACLIB_STUB1(callback)
#  define YACLIB_DEBUG(cond, message) YACLIB_STUB2(cond, message)
#  define YACLIB_ASSERT(cond) YACLIB_STUB1(!(cond))
#  define YACLIB_PURE_VIRTUAL() YACLIB_STUB1(true)
#endif
