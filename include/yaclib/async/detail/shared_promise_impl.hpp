#pragma once

namespace yaclib {
template <typename V, typename E>
template <typename Type>
void SharedPromise<V, E>::Set(Type&& value) && {
  static_assert(std::is_constructible_v<Result<V, E>, Type>, "TODO(MBkkt): Add message");
  auto core = std::exchange(_core, nullptr);
  while (core != nullptr) {
    core->Set(std::forward<Type>(value));
    core = core->next;
  }
}


}
