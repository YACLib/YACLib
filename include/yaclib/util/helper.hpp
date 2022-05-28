#pragma once

#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

template <typename ObjectType, typename PtrType = ObjectType, typename... Args>
IntrusivePtr<PtrType> MakeUnique(Args&&... args) {
  return {NoRefTag{}, new detail::UniqueCounter<ObjectType>{std::forward<Args>(args)...}};
}

template <typename ObjectType, typename PtrType = ObjectType, typename... Args>
IntrusivePtr<PtrType> MakeIntrusive(Args&&... args) {
  return {NoRefTag{}, new detail::AtomicCounter<ObjectType>{1, std::forward<Args>(args)...}};
}

}  // namespace yaclib
