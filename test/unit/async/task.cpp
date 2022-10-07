#include <yaclib/lazy/make.hpp>
#include <yaclib/lazy/schedule.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/result.hpp>

#include <thread>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Task, Simple) {
  std::size_t called = 0;
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

  yaclib::FairThreadPool tp{1};
  EXPECT_TRUE(task.Valid());
  yaclib::FutureOn<> future = std::move(task).ToFuture(tp);
  EXPECT_FALSE(task.Valid());
  std::ignore = std::move(future).Get();

  EXPECT_EQ(called, 6);

  tp.Stop();
  tp.Wait();
}

TEST(Task, Simple2) {
  yaclib::FairThreadPool tp{1};

  std::size_t called = 0;
  yaclib::Task<> task = yaclib::Schedule(tp,
                                         [&] {
                                           called += 1;
                                         })
                          .Then([&] {
                            called += 2;
                          })
                          .Then(tp, [&] {
                            called += 3;
                          });
  EXPECT_TRUE(task.Valid());
  yaclib::Future<> future = std::move(task).ToFuture();
  EXPECT_FALSE(task.Valid());
  std::ignore = std::move(future).Get();

  EXPECT_EQ(called, 6);

  tp.Stop();
  tp.Wait();
}

TEST(Task, Cancel) {
  yaclib::FairThreadPool tp{1};

  std::size_t called = 0;
  std::ignore = yaclib::MakeTask(1);
  yaclib::Task<> task1 = yaclib::Schedule([&] {
                           called += 1;
                         })
                           .Then([&] {
                             called += 2;
                           })
                           .Then([&] {
                             called += 3;
                           });
  yaclib::Task<> task2 = yaclib::Schedule(tp,
                                          [&] {
                                            called += 1;
                                          })
                           .Then([&] {
                             called += 2;
                           })
                           .Then(tp, [&] {
                             called += 3;
                           });

  EXPECT_TRUE(task1.Valid());
  std::move(task1).Cancel();
  EXPECT_FALSE(task1.Valid());
  EXPECT_EQ(called, 0);

  EXPECT_TRUE(task2.Valid());
  std::move(task2).Cancel();
  EXPECT_FALSE(task2.Valid());
  EXPECT_EQ(called, 0);

  tp.Stop();
  tp.Wait();
  EXPECT_EQ(called, 0);
}

}  // namespace
}  // namespace test
