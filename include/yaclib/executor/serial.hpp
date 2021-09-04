#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

IExecutorPtr MakeSerial(IExecutorPtr executor);

}  // namespace yaclib::executor
