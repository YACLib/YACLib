#pragma once

#include <functional>
#include <string_view>

using callback_type = std::function<void(std::string_view file, std::size_t line, std::string_view function,
                                         std::string_view condition, std::string_view message)>;

void SetErrorCallback(callback_type callback);

void SetInfoCallback(callback_type callback);

#ifndef YACLIB_FUNCTION_NAME
[[maybe_unused]] inline void FuncHelper() noexcept {
#ifdef __PRETTY_FUNCTION__
#define YACLIB_FUNCTION_NAME 2
#elif defined(__FUNCSIG__)
#define YACLIB_FUNCTION_NAME 1
#else
#define YACLIB_FUNCTION_NAME 0
#endif
}
#if YACLIB_FUNCTION_NAME == 2
#define YACLIB_FUNCTION_NAME __PRETTY_FUNCTION__
#elif YACLIB_FUNCTION_NAME == 1
#define YACLIB_FUNCTION_NAME __FUNCSIG__
#else
#define YACLIB_FUNCTION_NAME __func__
#endif
#endif

#define YACLIB_ERROR(cond, message)                                                                                    \
  do {                                                                                                                 \
    if (cond) {                                                                                                        \
      LogError(__FILE__, __LINE__, YACLIB_FUNCTION_NAME, (#cond), message);                                            \
    }                                                                                                                  \
  } while (false)

#define YACLIB_INFO(cond, message)                                                                                     \
  do {                                                                                                                 \
    if (cond) {                                                                                                        \
      LogInfo(__FILE__, __LINE__, YACLIB_FUNCTION_NAME, (#cond), message);                                             \
    }                                                                                                                  \
  } while (false)

void LogError(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
              std::string_view message);

void LogInfo(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
             std::string_view message);
