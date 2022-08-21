#pragma once

#include <yaclib/exe/executor.hpp>

namespace yaclib {

/**
 * Strand is the asynchronous analogue of a mutex
 *
 * It guarantees that the tasks scheduled for it will be executed strictly sequentially.
 * Strand itself does not have its own threads, it decorates another executor and uses it to run its tasks.
 * \param e executor to decorate
 * \return pointer to new Strand instance
 */
IExecutorPtr MakeStrand(IExecutorPtr e);

}  // namespace yaclib
