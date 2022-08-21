#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/fwd.hpp>

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
 * IExecutorPtr executor = &MakeInline();
 * if (...) {
 *   executor = MakeThreadPool(4);
 * }
 * executor->Submit(task);
 * \endcode
 */
IExecutor& MakeInline() noexcept;

IExecutor& MakeInline(StopTag) noexcept;

}  // namespace yaclib
