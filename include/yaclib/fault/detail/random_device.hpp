#pragma once

#include <random>
#include <string>

namespace yaclib::detail::thread {

// TODO(myannyax) Refactor this shit

class RandomDevice {
 public:
  using result_type = std::mt19937_64::result_type;

  RandomDevice();

  explicit RandomDevice(const std::string& /*token*/);

  result_type operator()() noexcept;

  [[nodiscard]] double entropy() const noexcept;

  static constexpr result_type min();

  static constexpr result_type max();

  ~RandomDevice() = default;

  void reset();

  RandomDevice(const RandomDevice&) = delete;

  RandomDevice& operator=(const RandomDevice&) = delete;

 private:
  std::mt19937_64 _eng;
};

}  // namespace yaclib::detail::thread
