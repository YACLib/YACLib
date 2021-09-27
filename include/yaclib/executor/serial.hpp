#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

/**
 * Serial is the asynchronous analogue of a mutex
 *
 * It guarantees that the tasks scheduled for it will be executed strictly sequentially.
 * Serial itself does not have its own threads, it decorates another executor and uses it to run its tasks.
 * \param e executor to decorate
 * \return pointer to new Serial instance
 */
IExecutorPtr MakeSerial(IExecutorPtr e);

}  // namespace yaclib::executor
