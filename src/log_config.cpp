#include <yaclib/log_config.hpp>

static callback_type* g_error_callback = nullptr;
static callback_type* g_info_callback = nullptr;

void SetErrorCallback(callback_type callback) {
  auto* func_ref = new callback_type(std::move(callback));
  auto* prev_callback = g_error_callback;
  g_error_callback = func_ref;
  delete prev_callback;
}

void SetInfoCallback(callback_type callback) {
  auto* func_ref = new callback_type(std::move(callback));
  auto* prev_callback = g_info_callback;
  g_info_callback = func_ref;
  delete prev_callback;
}

void LogError(bool condition, std::string_view message, std::string_view file, size_t line, std::string_view function) {
  if (!condition) {
    return;
  }
  if (g_error_callback == nullptr) {
    return;
  }
#ifdef YACLIB_LOG_ERROR
  g_error_callback->operator()(message, file, line, function);
#endif
}

void LogInfo(bool condition, std::string_view message, std::string_view file, size_t line, std::string_view function) {
  if (!condition) {
    return;
  }
  if (g_info_callback == nullptr) {
    return;
  }
#ifdef YACLIB_LOG_ERROR
  g_info_callback->operator()(message, file, line, function);
#elif defined(YACLIB_LOG_INFO)
  g_info_callback->operator()(message, file, line, function);
#endif
}
