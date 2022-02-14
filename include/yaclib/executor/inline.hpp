#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib {

/**
 * Get Inline executor singleton object.
 *
 * This executor immediately executes given Callable object in the same OS thread without any overhead.
 * So it always return false from Submit, we only call Call, no submit
 * \note This object is useful as safe default executor value. See example.
 *
 * \code
 * auto task = [] {};
 *
 * // Without inline executor:
 * IExecutorPtr executor = nullptr;
 * if (...) {
 *   executor = MakeThreadPool(4);
 * }
 * if (executor) {
 *   executor->Submit(task);
 * } else {
 *   a();
 * }
 *
 * // With inline executor:
 * IExecutorPtr executor = MakeInlineExecutor();
 * if (...) {
 *   executor = MakeThreadPool(4);
 * }
 * executor->Submit(task);
 * \endcode
 */
IExecutorPtr MakeInline() noexcept;

}  // namespace yaclib
