#include <yaclib/log.hpp>

#include <array>
#include <cstddef>

namespace yaclib::detail {

static std::array<LogCallback, static_cast<std::size_t>(LogLevel::Count)> sCallbacks = {};

void LogMessage(LogLevel level, std::string_view file, std::size_t line, std::string_view func,
                std::string_view condition, std::string_view message) noexcept {
  if (const auto callback = sCallbacks[static_cast<std::size_t>(level)]; callback != nullptr) {
    callback(file, line, func, condition, message);
  }
}

void SetCallback(LogLevel level, LogCallback callback) noexcept {
  sCallbacks[static_cast<std::size_t>(level)] = std::move(callback);
}

}  // namespace yaclib::detail
