#include <vector>
#include <yaclib_std/thread>
#include <yaclib_std/thread_local>

#include <gtest/gtest.h>

YACLIB_THREAD_LOCAL_PTR(int) tls_int_ptr;
YACLIB_THREAD_LOCAL_PTR(std::vector<int>) tls_vector_ptr;

TEST(FiberThreadLocal, Simple) {
  ASSERT_TRUE(!tls_int_ptr);
  int val = 3;
  tls_int_ptr = &val;
  ASSERT_TRUE(tls_int_ptr);
  ASSERT_TRUE(*tls_int_ptr == 3);
  int kek[3];
  tls_int_ptr = kek;
  ASSERT_TRUE(tls_int_ptr);
  tls_int_ptr[1] = 2;
  ASSERT_TRUE(tls_int_ptr[1] == 2);
  tls_int_ptr = nullptr;
  ASSERT_TRUE(!tls_int_ptr);
  std::vector<int> test_test;
  tls_vector_ptr = &test_test;
  tls_vector_ptr->push_back(3);
  ASSERT_TRUE((*tls_vector_ptr)[0] == 3);
  tls_vector_ptr->clear();
  ASSERT_TRUE(tls_vector_ptr == &test_test);
  ASSERT_TRUE(tls_vector_ptr != nullptr);
  ASSERT_TRUE(tls_int_ptr < tls_int_ptr || tls_int_ptr >= tls_int_ptr);
  ASSERT_TRUE(tls_vector_ptr->empty());
}
