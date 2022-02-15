#include <yaclib/log_config.hpp>

#include <utility>

static callback_type g_error_callback = nullptr;
static callback_type g_info_callback = nullptr;

void SetErrorCallback(callback_type callback) {
  g_error_callback = std::move(callback);
}

void SetInfoCallback(callback_type callback) {
  g_info_callback = std::move(callback);
}

void LogError(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
              std::string_view message) {
  if (g_error_callback == nullptr) {
    return;
  }
#ifdef YACLIB_LOG_ERROR
  g_error_callback(file, line, function, condition, message);
#endif
}

void LogInfo(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
             std::string_view message) {
  if (g_info_callback == nullptr) {
    return;
  }
#ifdef YACLIB_LOG_INFO
  g_info_callback(file, line, function, condition, message);
#endif
}
