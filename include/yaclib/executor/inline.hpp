#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

/**
 * \brief Get Inline executor singleton object.
 * This executor immediately executes given Callable object in the same OS thread without any overhead.
 * \note This object is useful as safe default executor value. See example.
 *
 * \code
 * auto task = [] { return 0; };
 *
 * // Without inline executor:
 * IExecutorPtr executor = nullptr;
 * if (...) {
 *   executor = MakeThreadPool(4);
 * }
 * if (executor) {
 *   executor->Execute(task);
 * } else {
 *   a();
 * }
 * // With inline executor:
 * IExecutorPtr executor{MakeInlineExecutor()};
 * ...
 * executor->Execute(task);
 * \endcode
 */
IExecutorPtr MakeInline() noexcept;

}  // namespace yaclib::executor
