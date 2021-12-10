#include <yaclib/lazy/run.hpp>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;

TEST(Simple, Simple) {
  LazyRun(MakeInline(), [] {
    std::cout << "1" << std::endl;
    return 1;
  }).Get();
}

}  // namespace
