#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>

#include <array>
#include <cstddef>
#include <new>

#include <gtest/gtest.h>

struct LogEntry {
  enum class Type : std::size_t {
    Empty = 0,
    New = 1,
    Delete = 2,
  };

  Type type{Type::Empty};
  void* pointer{nullptr};
  std::size_t size{0};
};

static const char* ToString(LogEntry::Type type) noexcept {
  switch (type) {
    case LogEntry::Type::Empty:
      return "empty";
    case LogEntry::Type::New:
      return "new";
    case LogEntry::Type::Delete:
      return "delete";
    default:
      return "";
  }
}

template <std::size_t N>
struct Log {
  void Add(LogEntry::Type type, void* pointer, std::size_t size) noexcept {
    if (!enabled) {
      return;
    }
    entries[position] = LogEntry{type, pointer, size};
    if (++position == N) {
      position = 0;
    }
  }

  void Reset() noexcept {
    entries.fill({});
    position = 0;
    enabled = false;
  }

  struct [[nodiscard]] Guard {
    Guard(Log& log) noexcept : _log{log} {
      _log.enabled = true;
    }

    ~Guard() noexcept {
      _log.enabled = false;
    }

   private:
    Log& _log;
  };

  Guard Enable() noexcept {
    return {*this};
  }

  std::array<LogEntry, N> entries;
  std::size_t position{0};
  mutable bool enabled{false};
};

template <std::size_t N>
std::ostream& operator<<(std::ostream& out, const Log<N>& log) {
  auto was_enabled = std::exchange(log.enabled, false);
  out << "length: " << log.position << "\n";
  for (std::size_t i = 0; i != log.position; ++i) {
    auto& e = log.entries[i];
    out << "type: " << ToString(e.type) << " pointer: " << e.pointer << " size: " << e.size << "\n";
  }
  log.enabled = was_enabled;
  return out;
}

static Log<100> gLog;

void* operator new(std::size_t sz) {
  if (sz == 0) {
    ++sz;
  }
  if (void* ptr = std::malloc(sz)) {
    gLog.Add(LogEntry::Type::New, ptr, sz);
    return ptr;
  }
  throw std::bad_alloc{};
}

void operator delete(void* pointer, std::size_t size) noexcept {
  gLog.Add(LogEntry::Type::Delete, pointer, size);
  std::free(pointer);
}

void operator delete(void* pointer) noexcept {
  gLog.Add(LogEntry::Type::Delete, pointer, 0);
  std::free(pointer);
}

namespace test {
namespace {

// TODO(MBkkt) add additional sync ThenInline before DetachInline

TEST(Dealloc, SyncLinear) {
  gLog.Reset();
  bool valid = true;
  std::size_t x = 0;
  {
    auto guard = gLog.Enable();
    auto [f, p] = yaclib::MakeContract();
    std::move(f)
      .ThenInline([&] {
        valid &= x == 0;
        x += 10;
      })
      .ThenInline([&] {
        valid &= x == 10;
        x += 100;
      })
      .DetachInline([&] {
        valid &= x == 110;
        x += 1000;
      });
    std::move(p).Set();
  }
  EXPECT_TRUE(valid);
  EXPECT_EQ(x, 1110);
  valid = gLog.position == 8;
  for (std::size_t i = 0; valid && i != gLog.position; ++i) {
    if (i < 4) {
      valid &= gLog.entries[i].type == LogEntry::Type::New;
    } else {
      valid &= gLog.entries[i].type == LogEntry::Type::Delete;
      valid &= gLog.entries[i].pointer == gLog.entries[i - 4].pointer;
    }
    ASSERT_TRUE(valid) << "Incorrect position: " << i << ", all log: \n" << gLog;
  }
  EXPECT_TRUE(valid) << gLog;
}

TEST(Dealloc, AsyncLinear) {
  gLog.Reset();
  bool valid = true;
  std::size_t x = 0;
  {
    auto guard = gLog.Enable();
    auto [f, p] = yaclib::MakeContract();
    std::move(f)
      .ThenInline([&] {
        valid &= x == 0;
        x += 10;
        return yaclib::Run(yaclib::MakeInline(), [&] {
          valid &= x == 10;
          x += 100;
        });
      })
      .DetachInline([&] {
        valid &= x == 110;
        x += 1000;
      });
    std::move(p).Set();
  }
  EXPECT_TRUE(valid);
  EXPECT_EQ(x, 1110);
  valid = gLog.position == 8;
  for (std::size_t i = 0; valid && i != gLog.position; ++i) {
    if (i < 4) {
      valid &= gLog.entries[i].type == LogEntry::Type::New;
    } else {
      valid &= gLog.entries[i].type == LogEntry::Type::Delete;
      if (i == 4) {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 4].pointer;
      } else if (i == 6 || i == 7) {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 5].pointer;
      } else {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 2].pointer;
      }
    }
    ASSERT_TRUE(valid) << "Incorrect position: " << i << ", all log: \n" << gLog;
  }
  EXPECT_TRUE(valid) << gLog;
}

TEST(Dealloc, SyncFast) {
  gLog.Reset();
  bool valid = true;
  std::size_t x = 0;
  {
    auto guard = gLog.Enable();
    yaclib::MakeFuture()
      .ThenInline([&] {
        valid &= x == 0;
        x += 10;
      })
      .ThenInline([&] {
        valid &= x == 10;
        x += 100;
      })
      .DetachInline([&] {
        valid &= x == 110;
        x += 1000;
      });
  }
  EXPECT_TRUE(valid);
  EXPECT_EQ(x, 1110);
  valid = gLog.position == 8;
  for (std::size_t i = 0; valid && i != gLog.position; ++i) {
    if (i < 2 || i == 3 || i == 5) {
      valid &= gLog.entries[i].type == LogEntry::Type::New;
    } else {
      valid &= gLog.entries[i].type == LogEntry::Type::Delete;
      if (i == 2 || i == 7) {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 2].pointer;
      } else {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 3].pointer;
      }
    }
    ASSERT_TRUE(valid) << "Incorrect position: " << i << ", all log: \n" << gLog;
  }
  EXPECT_TRUE(valid) << gLog;
}

TEST(Dealloc, AsyncFast) {
  gLog.Reset();
  bool valid = true;
  std::size_t x = 0;
  {
    auto guard = gLog.Enable();
    yaclib::MakeFuture()
      .ThenInline([&] {
        valid &= x == 0;
        x += 10;
        return yaclib::Run(yaclib::MakeInline(), [&] {
          valid &= x == 10;
          x += 100;
        });
      })
      .DetachInline([&] {
        valid &= x == 110;
        x += 1000;
      });
  }
  EXPECT_TRUE(valid);
  EXPECT_EQ(x, 1110);
  valid = gLog.position == 8;
  for (std::size_t i = 0; valid && i != gLog.position; ++i) {
    if (i < 3 || i == 5) {
      valid &= gLog.entries[i].type == LogEntry::Type::New;
    } else {
      valid &= gLog.entries[i].type == LogEntry::Type::Delete;
      if (i == 3) {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 3].pointer;
      } else if (i == 4 || i == 7) {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 2].pointer;
      } else {
        valid &= gLog.entries[i].pointer == gLog.entries[i - 5].pointer;
      }
    }
    ASSERT_TRUE(valid) << "Incorrect position: " << i << ", log: \n" << gLog;
  }
  EXPECT_TRUE(valid) << gLog;
}

}  // namespace
}  // namespace test
