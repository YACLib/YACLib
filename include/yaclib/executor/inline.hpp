#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

IExecutorPtr MakeInlineExecutor() noexcept;

}  // namespace yaclib::executor
