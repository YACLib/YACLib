#pragma once

#include <yaclib/config.hpp>

#include <memory>
#include <type_traits>

namespace test {

void InitLog() noexcept;

void InitFault();

// Convenient helper for simulating 'try/catch/finally' semantic
template <typename Func>
class [[nodiscard]] Finally {
 public:
  static_assert(std::is_nothrow_invocable_v<Func>);

  // If you need some of it, please use absl::Cleanup
  Finally(Finally&&) = delete;
  Finally(const Finally&) = delete;
  Finally& operator=(Finally&&) = delete;
  Finally& operator=(const Finally&) = delete;

  Finally(Func&& func) : func_{std::move(func)} {
  }

  ~Finally() noexcept {
    func_();
  }

 private:
  YACLIB_NO_UNIQUE_ADDRESS Func func_;
};

}  // namespace test
