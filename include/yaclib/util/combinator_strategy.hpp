#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/fail_policy.hpp>

namespace yaclib {

// Every combinator strategy shall be parametrized with the following template arguments
// template <FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
// struct ExampleStrategy;
//
// It must also have
// using PromiseType = Promise<OutputValue, OutputError>
// for the combinator to know the type of Promise to pass to the strategy

// Every combinator stategy must have the following two static variables defined:
// - ConsumePolicy kConsumePolicy = None | Unordered | Static | Dynamic;
// - CorePolicy kCorePolicy = Owned | Managed;

// This defines what kind of the Consume method will be called
// All kinds of Consume will have the `target` parameter, which will be a Core or a Result,
// depending on CorePolicy. Cores should usually be taken by lvalue reference, while
// Results should be taken by universal refernce
enum class ConsumePolicy {
  None,       // no Strategy::Consume() method will be called
  Unordered,  // Strategy::Consume(target) will be called
  Static,     // Strategy::Consume<Index>(target) will be called
  Dynamic,    // Strategy::Consume(index, target) will be called
};

// Owned means the strategy owns the cores and manages them by itself
// while Managed means that the Results from the Cores will be passed
// to the strategy in Strategy::Consume()
// but the Cores themselves will be managed on the outside
enum class CorePolicy {
  // For every Core, Strategy::Register(index, core) will be called before setting the callbacks
  // Also, Strategy::Consume, if present, will receive the Core
  Owned,
  // Strategy::Consume, if present, will receive the Result
  Managed,
};

// The related code is located in async/when/when.hpp

}  // namespace yaclib
