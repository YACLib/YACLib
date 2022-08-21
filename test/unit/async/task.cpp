#include <yaclib/async/task.hpp>
#include <yaclib/exe/thread_pool.hpp>
#include <yaclib/util/result.hpp>

#include <thread>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Task, Simple) {
  size_t called = 0;
  yaclib::Task<> task = yaclib::Schedule([&] {
                          called += 1;
                        })
                          .On(nullptr)
                          .Then([&] {
                            called += 2;
                          })
                          .Then([&] {
                            called += 3;
                          });

  auto tp = yaclib::MakeThreadPool(1);
  EXPECT_TRUE(task.Valid());
  yaclib::FutureOn<> future = std::move(task).ToFuture(*tp);
  EXPECT_FALSE(task.Valid());
  std::ignore = std::move(future).Get();

  EXPECT_EQ(called, 6);

  tp->Stop();
  tp->Wait();
}

TEST(Task, Simple2) {
  auto tp = yaclib::MakeThreadPool(1);

  size_t called = 0;
  yaclib::Task<> task = yaclib::Schedule(*tp,
                                         [&] {
                                           called += 1;
                                         })
                          .Then([&] {
                            called += 2;
                          })
                          .On(*tp)
                          .Then([&] {
                            called += 3;
                          });
  EXPECT_TRUE(task.Valid());
  yaclib::Future<> future = std::move(task).ToFuture();
  EXPECT_FALSE(task.Valid());
  std::ignore = std::move(future).Get();

  EXPECT_EQ(called, 6);

  tp->Stop();
  tp->Wait();
}

TEST(Task, Cancel) {
  auto tp = yaclib::MakeThreadPool(1);

  size_t called = 0;
  std::ignore = yaclib::MakeTask(1);
  yaclib::Task<> task1 = yaclib::Schedule([&] {
                           called += 1;
                         })
                           .On(nullptr)
                           .Then([&] {
                             called += 2;
                           })
                           .Then([&] {
                             called += 3;
                           });
  yaclib::Task<> task2 = yaclib::Schedule(*tp,
                                          [&] {
                                            called += 1;
                                          })
                           .Then([&] {
                             called += 2;
                           })
                           .On(*tp)
                           .Then([&] {
                             called += 3;
                           });
  EXPECT_TRUE(task1.Valid());
  std::move(task1).Cancel();
  EXPECT_FALSE(task1.Valid());
  EXPECT_TRUE(task2.Valid());
  std::move(task2).Cancel();
  EXPECT_FALSE(task2.Valid());
  EXPECT_EQ(called, 0);
  tp->Stop();
  tp->Wait();
  EXPECT_EQ(called, 0);
}

}  // namespace
}  // namespace test
