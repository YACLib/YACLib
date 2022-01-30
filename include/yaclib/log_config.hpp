#pragma once

#include <functional>
#include <string_view>

using callback_type = std::function<void(std::string_view, std::string_view, size_t, std::string_view)>;

void SetErrorCallback(callback_type callback);

void SetInfoCallback(callback_type callback);

#ifdef __PRETTY_FUNCTION__
#define YACLIB_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define YACLIB_FUNCTION_NAME __FUNCSIG__
#elif defined(__func__)
#define YACLIB_FUNCTION_NAME __func__
#else
#define YACLIB_FUNCTION_NAME __FUNCTION__
#endif

#define YACLIB_ERROR(cond, message)                                                                                    \
  do {                                                                                                                 \
    LogError(cond, message, __FILE__, __LINE__, YACLIB_FUNCTION_NAME);                                                 \
  } while (false)

#define YACLIB_INFO(cond, message)                                                                                     \
  do {                                                                                                                 \
    LogInfo(cond, message, __FILE__, __LINE__, YACLIB_FUNCTION_NAME);                                                  \
  } while (false)

void LogError(bool condition, std::string_view message, std::string_view file, size_t line, std::string_view function);

void LogInfo(bool condition, std::string_view message, std::string_view file, size_t line, std::string_view function);
