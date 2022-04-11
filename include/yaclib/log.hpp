#pragma once

#include <string_view>

namespace yaclib {

// TODO(MBkkt) Maybe add callbacks overload for source_location, stacktrace arguments?

using LogCallback = void (*)(std::string_view file, std::size_t line, std::string_view function,
                             std::string_view condition, std::string_view message);

namespace detail {

enum class LogLevel : char {
  Debug = 0,
  Info = 1,
  Error = 2,
  Count = 3,
};

void SetCallback(LogLevel level, LogCallback callback) noexcept;

void LogMessage(LogLevel level, std::string_view file, std::size_t line, std::string_view function,
                std::string_view condition, std::string_view message);

}  // namespace detail
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

#define YACLIB_LOG_MESSAGE(level, cond, message)                                                                       \
  do {                                                                                                                 \
    if (!!(cond)) {                                                                                                    \
      ::yaclib::detail::LogMessage(level, __FILE__, __LINE__, YACLIB_FUNCTION_NAME, #cond, (message));                 \
    }                                                                                                                  \
  } while (false)

#define YACLIB_SET_CALLBACK(level, callback)                                                                           \
  do {                                                                                                                 \
    ::yaclib::detail::SetCallback(level, (callback));                                                                  \
  } while (false)

#ifdef NDEBUG
#  define YACLIB_STUB1(first) ((void)0)
#  define YACLIB_STUB2(first, second) ((void)0)
#else
#  define YACLIB_STUB1(first)                                                                                          \
    do {                                                                                                               \
      (void)(first);                                                                                                   \
    } while (false)
#  define YACLIB_STUB2(first, second)                                                                                  \
    do {                                                                                                               \
      (void)(first);                                                                                                   \
      (void)(second);                                                                                                  \
    } while (false)
#endif

#ifdef YACLIB_LOG_ERROR
#  define YACLIB_INIT_ERROR(callback) YACLIB_SET_CALLBACK(::yaclib::detail::LogLevel::Error, callback)
#  define YACLIB_ERROR(cond, message) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Error, cond, message)
#else
#  define YACLIB_INIT_ERROR(callback) YACLIB_STUB1(callback)
#  define YACLIB_ERROR(cond, message) YACLIB_STUB2(cond, message)
#endif

#ifdef YACLIB_LOG_INFO
#  define YACLIB_INIT_INFO(callback) YACLIB_SET_CALLBACK(::yaclib::detail::LogLevel::Info, callback)
#  define YACLIB_INFO(cond, message) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Info, cond, message)
#else
#  define YACLIB_INIT_INFO(callback) YACLIB_STUB1(callback)
#  define YACLIB_INFO(cond, message) YACLIB_STUB2(cond, message)
#endif

#ifdef YACLIB_LOG_DEBUG
#  define YACLIB_INIT_DEBUG(callback) YACLIB_SET_CALLBACK(::yaclib::detail::LogLevel::Debug, callback)
#  define YACLIB_DEBUG(cond, message) YACLIB_LOG_MESSAGE(::yaclib::detail::LogLevel::Debug, cond, message)
#else
#  define YACLIB_INIT_DEBUG(callback) YACLIB_STUB1(callback)
#  define YACLIB_DEBUG(cond, message) YACLIB_STUB2(cond, message)
#endif
