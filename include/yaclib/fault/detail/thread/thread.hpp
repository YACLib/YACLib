#pragma once

#include <yaclib/fiber/coroutine.hpp>
#include <yaclib/log_config.hpp>
#include <yaclib/util/func.hpp>

#include <functional>
#include <thread>

namespace yaclib::detail {

class Thread {
 public:
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  using id = std::thread::id;
  using native_handle_type = std::thread::native_handle_type;

  Thread() noexcept;

  template <class Fp, class... Args>
  inline explicit Thread(Fp&& f, Args&&... args)
      : _impl([&]() {
          yaclib::IFuncPtr func = yaclib::MakeFunc([&] {
            f(std::move(args)...);
          });
          yaclib::Coroutine coroutine{func};
          while (!coroutine.IsCompleted()) {
            coroutine();
          }
        }) {
  }

  Thread(Thread&& t) noexcept;

  ~Thread() = default;

  Thread& operator=(Thread&& t) noexcept;

  void swap(Thread& t) noexcept;
  [[nodiscard]] bool joinable() const noexcept;
  void join();
  void detach();

  [[nodiscard]] id get_id() const noexcept;

  native_handle_type native_handle() noexcept;

  // TODO(myannyax) don't use auto?
  static auto hardware_concurrency() noexcept;

 private:
  std::thread _impl;
};

extern const Thread::id kInvalidThreadId;

}  // namespace yaclib::detail
