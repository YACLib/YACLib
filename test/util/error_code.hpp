#pragma once

#include <yaclib/util/result.hpp>

#include <system_error>

namespace test {

struct LikeErrorCode : std::error_code {
  using std::error_code::error_code;

  LikeErrorCode(yaclib::StopTag /*tag*/) : std::error_code{std::make_error_code(std::errc::operation_canceled)} {
  }

  LikeErrorCode(LikeErrorCode&&) noexcept = default;
  LikeErrorCode(const LikeErrorCode&) noexcept = default;
  LikeErrorCode& operator=(LikeErrorCode&&) noexcept = default;
  LikeErrorCode& operator=(const LikeErrorCode&) noexcept = default;

  const char* What() const noexcept {
    return this->category().name();
  }
};

}  // namespace test
