#include <vector>
#include <yaclib_std/thread>
#include <yaclib_std/thread_local>

#include <gtest/gtest.h>

static YACLIB_THREAD_LOCAL_PTR(int) tls_int_ptr;
static YACLIB_THREAD_LOCAL_PTR(std::vector<int>) tls_vector_ptr;

TEST(FiberThreadLocal, Simple) {
  EXPECT_TRUE(!tls_int_ptr);
  int val = 3;
  tls_int_ptr = &val;
  EXPECT_TRUE(tls_int_ptr);
  EXPECT_TRUE(*tls_int_ptr == 3);
  int kek[3];
  tls_int_ptr = kek;
  EXPECT_TRUE(tls_int_ptr);
  tls_int_ptr[1] = 2;
  EXPECT_TRUE(tls_int_ptr[1] == 2);
  tls_int_ptr = nullptr;
  EXPECT_TRUE(!tls_int_ptr);
  std::vector<int> test_test;
  tls_vector_ptr = &test_test;
  tls_vector_ptr->push_back(3);
  EXPECT_TRUE((*tls_vector_ptr)[0] == 3);
  tls_vector_ptr->clear();
  EXPECT_TRUE(tls_vector_ptr == &test_test);
  EXPECT_TRUE(tls_vector_ptr != nullptr);
  EXPECT_TRUE(tls_int_ptr < tls_int_ptr || tls_int_ptr >= tls_int_ptr);
  EXPECT_TRUE(tls_vector_ptr->empty());
}
