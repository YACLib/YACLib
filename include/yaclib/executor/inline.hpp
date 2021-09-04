#pragma once

#include <yaclib/executor/executor.hpp>

namespace yaclib::executor {

/**
 * \brief Get Inline executor singleton object.
 * This executor immediately executes given Callable object in the same OS thread without any overhead.
 * \note This object is useful as safe default executor value. See example.
 * \example
 * auto task = [] { return 0; };
 * IExecutorPtr executor = nullptr;
 * if (rand() == 1) {
 *   executor = MakeThreadPool(4);
 * }
 * // Without inline executor:
 * if (executor) {
 *   executor->Execute(task);
 * } else {
 *   a();
 * }
 * // With inline executor:
 * ...
 * IExecutorPtr executor{MakeInlineExecutor()};
 * ...
 * executor->Execute(task);
 */
IExecutorPtr MakeInline() noexcept;

}  // namespace yaclib::executor
