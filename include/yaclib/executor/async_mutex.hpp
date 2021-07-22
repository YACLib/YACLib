#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

IExecutorPtr MakeAsyncMutex(IExecutorPtr executor);

}  // namespace yaclib::executor
