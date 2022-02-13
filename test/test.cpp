#include <cstdio>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
#ifdef __GLIBCPP__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCPP__);
#endif
#ifdef __GLIBCXX__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCXX__);
#endif
#ifdef _LIBCPP_VERSION
  std::fprintf(stderr, "libc++: %d\n", _LIBCPP_VERSION);
#endif
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
