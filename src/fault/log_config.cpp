#include <yaclib/fault/log_config.hpp>

static std::atomic<callback_type*> g_fatal_callback;
static std::atomic<callback_type*> g_error_callback;
static std::atomic<callback_type*> g_ignore_callback;

void SetLogFatalCallback(callback_type callback) {
  auto* func_ref = new callback_type(std::move(callback));
  auto* prev_callback = g_fatal_callback.load();
  g_fatal_callback.store(func_ref);
  delete prev_callback;
}

void SetLogErrorCallback(callback_type callback) {
  auto* func_ref = new callback_type(std::move(callback));
  auto* prev_callback = g_error_callback.load();
  g_error_callback.store(func_ref);
  delete prev_callback;
}

void SetLogIgnoreCallback(callback_type callback) {
  auto* func_ref = new callback_type(std::move(callback));
  auto* prev_callback = g_ignore_callback.load();
  g_ignore_callback.store(func_ref);
  delete prev_callback;
}

void Log(bool condition, std::string_view message) {
  if (condition) {
    return;
  }
#ifdef YACLIB_ASSERT_FATAL
  g_fatal_callback.load()->operator()(message);
#elif YACLIB_ASSERT_ERROR
  g_error_callback.load()->operator()(message);
#else
  g_ignore_callback.load()->operator()(message);
#endif
}
