#pragma once

#include <util/error_code.hpp>

#include <yaclib/util/result.hpp>

#include <string>

#include <gtest/gtest.h>

namespace test {

template <typename T>
class Error : public testing::Test {
 public:
  using Type = T;
};

class ErrorNames {
 public:
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "std::exception_ptr";
      case 1:
        return "yaclib::StopError";
      case 2:
        return "std::error_code";
      default:
        return "unknown";
    }
  }
};

using ErrorTypes = testing::Types<std::exception_ptr, yaclib::StopError, LikeErrorCode>;

TYPED_TEST_SUITE(Error, ErrorTypes, ErrorNames);

}  // namespace test
