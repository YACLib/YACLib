#undef YACLIB_LOG_INFO
#define YACLIB_LOG_INFO
#include <yaclib/log.hpp>

#include <utility>

namespace yaclib {

static LogCallback g_info_callback = nullptr;

void SetInfoCallback(LogCallback callback) {
  g_info_callback = std::move(callback);
}

void detail::LogInfo(std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
                     std::string_view message) {
  // GCOVR_EXCL_START
  if (g_info_callback == nullptr) {
    return;
  }
  g_info_callback(file, line, function, condition, message);
  // GCOVR_EXCL_END
}
}  // namespace yaclib
