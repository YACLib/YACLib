#pragma once

#include <yaclib/algo/detail/func_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/algo/detail/shared_core.hpp>
#include <yaclib/algo/detail/unique_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/helper.hpp>

namespace yaclib::detail {

InlineCore& MakeDrop() noexcept;

class NoResultCore : public BaseCore {
 public:
  NoResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename T>
  void Store(T&&) noexcept {
  }

  void StoreCallback(InlineCore& callback) noexcept {
    BaseCore::StoreCallbackImpl(callback);
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return BaseCore::SetResultImpl<SymmetricTransfer, false>();
  }

  Callback _self;
};

enum class CoreType : unsigned char {
  None = 0,
  Run = 1 << 0,
  Detach = 1 << 1,
  FromUnique = 1 << 2,
  FromShared = 1 << 3,
  ToUnique = 1 << 4,
  ToShared = 1 << 5,
  Call = 1 << 6,
  Lazy = 1 << 7,
};

inline constexpr CoreType operator|(CoreType a, CoreType b) {
  return static_cast<CoreType>(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

inline constexpr CoreType operator&(CoreType a, CoreType b) {
  return static_cast<CoreType>(static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
}

constexpr bool IsRun(CoreType type) {
  return static_cast<bool>(type & CoreType::Run);
}

constexpr bool IsDetach(CoreType type) {
  return static_cast<bool>(type & CoreType::Detach);
}

constexpr bool IsFromUnique(CoreType type) {
  return static_cast<bool>(type & CoreType::FromUnique);
}

constexpr bool IsFromShared(CoreType type) {
  return static_cast<bool>(type & CoreType::FromShared);
}

constexpr bool IsToShared(CoreType type) {
  return static_cast<bool>(type & CoreType::ToShared);
}

constexpr bool IsToUnique(CoreType type) {
  return static_cast<bool>(type & CoreType::ToUnique);
}

constexpr bool IsCall(CoreType type) {
  return static_cast<bool>(type & CoreType::Call);
}

constexpr bool IsLazy(CoreType type) {
  return static_cast<bool>(type & CoreType::Lazy);
}

template <CoreType Type, typename V, typename E>
using ResultCoreT = std::conditional_t<IsDetach(Type), NoResultCore,
                                       std::conditional_t<IsToShared(Type), SharedCore<V, E>, UniqueCore<V, E>>>;

YACLIB_INLINE BaseCore* MoveToCaller(BaseCore* head) noexcept {
  YACLIB_ASSERT(head);
  while (head->next != nullptr) {
    auto* next = static_cast<BaseCore*>(head->next);
    head->next = nullptr;
    head = next;
  }
  return head;
}

enum class AsyncType {
  None,
  Unique,
  Shared,
};

template <typename Ret, typename Arg, typename E, typename Func, CoreType Type, AsyncType kAsync>
class Core : public ResultCoreT<Type, Ret, E>, public FuncCore<Func> {
  using F = FuncCore<Func>;
  using Storage = typename F::Storage;
  using Invoke = typename F::Invoke;

  static_assert(!(IsDetach(Type) && kAsync != AsyncType::None), "Detach cannot be Async, should be void");

 public:
  using Base = ResultCoreT<Type, Ret, E>;

  explicit Core(Func&& f) : F{std::forward<Func>(f)} {
    this->_self = {};
  }

 private:
  void Call() noexcept final {
    YACLIB_ASSERT(this->_self.unwrapping == 0);
    if constexpr (IsRun(Type)) {
      YACLIB_ASSERT(this->_self.caller == nullptr);
      if constexpr (is_invocable_v<Invoke>) {
        Loop(this, CallImpl<false>(Unit{}));  // optimization
      } else {
        Loop(this, CallImpl<false>(Result<Arg, E>{Unit{}}));
      }
    } else {
      YACLIB_ASSERT(this->_self.caller != this);
      auto& core = DownCast<ResultCore<Arg, E>>(*this->_self.caller);
      Loop(this, CallImpl<false>(core.template MoveOrConst<IsFromUnique(Type)>()));
    }
  }

  void Drop() noexcept final {
    Loop(this, CallImpl<false>(Result<Arg, E>{StopTag{}}));
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl([[maybe_unused]] InlineCore& caller) noexcept {
    auto async_done = [&] {
      YACLIB_ASSERT(kAsync != AsyncType::None);
      YACLIB_ASSERT(&caller == this || &caller == this->_self.caller);
      static constexpr bool AsyncShared = kAsync == AsyncType::Shared;
      auto& core = DownCast<ResultCore<Ret, E>>(*this->_self.caller);
      return Done<SymmetricTransfer, true>(core.template MoveOrConst<!AsyncShared>());
    };
    if constexpr (IsRun(Type)) {
      return async_done();
    } else {
      if constexpr (kAsync != AsyncType::None) {
        if (this->_self.unwrapping != 0) {
          return async_done();
        }
      }
      YACLIB_ASSERT(this->_self.caller == nullptr);
      this->_self.caller = &caller;
      DownCast<BaseCore>(caller).TransferExecutorTo<IsFromShared(Type)>(*this);
      if constexpr (IsFromShared(Type) && (IsCall(Type) || kAsync != AsyncType::None)) {
        // The callback can outlive all SharedFutures
        // We assume ownership here and release it in Done()
        caller.IncRef();
      }
      if constexpr (IsCall(Type)) {
        this->_executor->Submit(*this);
        return Noop<SymmetricTransfer>();
      } else {
        auto& core = DownCast<ResultCore<Arg, E>>(caller);
        return CallImpl<SymmetricTransfer>(core.template MoveOrConst<IsFromUnique(Type)>());
      }
    }
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif

  template <bool SymmetricTransfer, typename T>
  [[nodiscard]] YACLIB_INLINE auto CallImpl(T&& r) noexcept try {
    if constexpr (std::is_same_v<T, Unit> || is_invocable_v<Invoke, Result<Arg, E>>) {
      return CallResolveAsync<SymmetricTransfer>(std::forward<T>(r));
    } else {
      return CallResolveState<SymmetricTransfer>(std::forward<T>(r));
    }
  } catch (...) {
    return Done<SymmetricTransfer>(std::current_exception());
  }

  template <bool SymmetricTransfer, bool Async = false, typename T>
  [[nodiscard]] YACLIB_INLINE auto Done(T&& value) noexcept {
    // Order defined here is important:
    // 1. Save caller on stack
    // 2. Save return value to union where was caller
    // 3. Destroy and dealloc argument storage
    // 4. Destroy functor storage
    // 5. Make current core ready, maybe execute next callback
    // 3 and 4 can be executed in any order TODO(MBkkt) What order is better here?
    // Other steps cannot be reordered, examples:
    // [] (X&& x) -> X&& { touch x, then return it by rvalue reference }
    // [X x]   () -> X&& { touch x, then return it by rvalue reference }
    auto* caller = this->_self.caller;
    this->Store(std::forward<T>(value));

    // We decrease the reference count in the following cases:
    // If !Async (We are now not called by the async result of our callback), then:
    //   !Type & Run : We are not the first callback in the chain so there is a previous core AND:
    //     Type & FromUnique : The previous core is a UniqueCore, we always have ownership
    //     OR we have might have a SharedCore as the previous core and in that case:
    //       Type & Call : Our callback can outlive the previous core OR
    //       kAsync != AsyncType::None: Async operation of our callback can outlive the previous core
    //       - So in the last two cases, we assume ownership of the SharedCore beforehand and release it here
    // If Async, then we always have ownership of the async result of our callback
    if constexpr ((!IsRun(Type) && (IsFromUnique(Type) || IsCall(Type) || kAsync != AsyncType::None)) || Async) {
      caller->DecRef();
    }
    if constexpr (!Async) {
      this->_func.storage.~Storage();
    }
    return this->template SetResult<SymmetricTransfer>();
  }

  template <bool SymmetricTransfer, typename Result>
  [[nodiscard]] YACLIB_INLINE auto CallResolveState(Result&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<Invoke, Arg> || (std::is_void_v<Arg> && is_invocable_v<Invoke, Unit>)) {
      if (state == ResultState::Value) {
        return CallResolveAsync<SymmetricTransfer>(std::forward<Result>(r).Value());
      } else if (state == ResultState::Exception) {
        return Done<SymmetricTransfer>(std::forward<Result>(r).Exception());
      } else {
        YACLIB_ASSERT(state == ResultState::Error);
        return Done<SymmetricTransfer>(std::forward<Result>(r).Error());
      }
    } else {
      /**
       * TLDR: Before and after the "recovery" callback must have the same value type
       *
       * Why can't we use the above strategy for other Invoke?
       * Because user will not have compile error for this case:
       * MakeFuture()
       *   // Can't call next recovery callback, because our result is Ok, so we will skip next callback
       *   .ThenInline([](E/std::exception_ptr) -> yaclib::Result/Future<double> {
       *     throw std::runtime_error{""};
       *   })
       *   // Previous callback was skipped, so previous Result type is void (received from MakeFuture)
       *   // But here we need Result<double>, so it leeds error
       *   .ThenInline([](yaclib::Result<double>) {
       *     return 1;
       *   });
       */
      constexpr bool kIsException = is_invocable_v<Invoke, std::exception_ptr>;
      constexpr bool kIsError = is_invocable_v<Invoke, E>;
      static_assert(kIsException ^ kIsError, "Recovery callback should be invokable with std::exception_ptr or E");
      constexpr auto kState = kIsException ? ResultState::Exception : ResultState::Error;
      if (state == kState) {
        using T = std::conditional_t<kIsException, std::exception_ptr, E>;
        return CallResolveAsync<SymmetricTransfer>(std::get<T>(std::forward<Result>(r).Internal()));
      }
      return Done<SymmetricTransfer>(std::move(r));
    }
  }

  template <bool SymmetricTransfer, typename T>
  [[nodiscard]] YACLIB_INLINE auto CallResolveAsync(T&& value) {
    if constexpr (kAsync != AsyncType::None) {
      auto async = CallResolveVoid(std::forward<T>(value));
      // In the case of SharedFuture we also release here because the
      // ownership needs to be 'transferred' into *this
      auto* core = async.GetCore().Release();
      if constexpr (!IsRun(Type)) {
        this->_self.caller->DecRef();
        this->_self.unwrapping = 1;
      }
      this->_self.caller = core;
      this->_func.storage.~Storage();
      if constexpr (is_task_v<decltype(async)>) {
        core->StoreCallback(*this);
        return Step<SymmetricTransfer>(*this, *MoveToCaller(core));
      } else {
        return core->template SetInline<SymmetricTransfer>(*this);
      }
    } else {
      return Done<SymmetricTransfer>(CallResolveVoid(std::forward<T>(value)));
    }
  }

  template <typename T>
  [[nodiscard]] YACLIB_INLINE auto CallResolveVoid(T&& value) {
    constexpr bool kArgVoid = is_invocable_v<Invoke>;
    constexpr bool kRetVoid = std::is_void_v<invoke_t<Invoke, std::conditional_t<kArgVoid, void, T>>>;
    if constexpr (kRetVoid) {
      if constexpr (kArgVoid) {
        std::forward<Invoke>(this->_func.storage)();
      } else {
        std::forward<Invoke>(this->_func.storage)(std::forward<T>(value));
      }
      return Unit{};
    } else if constexpr (kArgVoid) {
      return std::forward<Invoke>(this->_func.storage)();
    } else {
      return std::forward<Invoke>(this->_func.storage)(std::forward<T>(value));
    }
  }
};

template <typename V, typename E, typename Func>
constexpr char Tag() noexcept {
  if constexpr (is_invocable_v<Func, Result<V, E>>) {
    return 1;
  } else if constexpr (is_invocable_v<Func, V>) {
    return 2;
  } else if constexpr (is_invocable_v<Func, E>) {
    return 3;
  } else if constexpr (is_invocable_v<Func, std::exception_ptr>) {
    return 4;
  } else if constexpr (is_invocable_v<Func, Unit>) {
    return 5;
  } else {
    return 0;
  }
}

template <typename V, typename E, typename Func, char Tag = Tag<V, E, Func>()>
struct Return;

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 1> final {
  using Type = invoke_t<Func, Result<V, E>>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 2> final {
  using Type = invoke_t<Func, V>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 3> final {
  using Type = invoke_t<Func, E>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 4> final {
  using Type = invoke_t<Func, std::exception_ptr>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 5> final {
  using Type = invoke_t<Func, Unit>;
};

template <CoreType CoreT, typename Arg, typename E, typename Func>
auto* MakeCore(Func&& f) {
  static_assert(!IsRun(CoreT) || std::is_void_v<Arg>, "It makes no sense to receive some value in first pipeline step");
  using AsyncRet = result_value_t<typename detail::Return<Arg, E, Func&&>::Type>;
  static_assert(!IsDetach(CoreT) || std::is_void_v<AsyncRet>,
                "It makes no sense to return some value in Detach, since no one will be able to use it");
  using Ret0 = result_value_t<async_value_t<task_value_t<AsyncRet>>>;
  using Ret = std::conditional_t<std::is_same_v<Ret0, Unit>, void, Ret0>;
  constexpr AsyncType kAsync = [] {
    if constexpr (is_future_base_v<AsyncRet> || is_task_v<AsyncRet>) {
      return AsyncType::Unique;
    } else if constexpr (is_shared_future_base_v<AsyncRet>) {
      return AsyncType::Shared;
    } else {
      return AsyncType::None;
    }
  }();
  // TODO(MBkkt) Think about inline/detach optimization
  using Core = detail::Core<Ret, Arg, E, Func&&, CoreT, kAsync>;
  if constexpr (IsToShared(CoreT)) {
    return MakeShared<Core>(detail::kSharedRefWithFuture, std::forward<Func>(f)).Release();
  } else {
    return MakeUnique<Core>(std::forward<Func>(f)).Release();
  }
}

template <CoreType CoreT, bool On, typename FromCorePtr, typename Func>
auto SetCallback(FromCorePtr&& core, IExecutor* executor, Func&& f) {
  using Arg = typename remove_cvref_t<FromCorePtr>::Value::Value;
  using E = typename remove_cvref_t<FromCorePtr>::Value::Error;

  static constexpr bool Unique = std::is_same_v<UniqueCorePtr<Arg, E>&, FromCorePtr>;
  static constexpr bool Shared = std::is_same_v<const SharedCorePtr<Arg, E>&, FromCorePtr>;
  static_assert(Unique || Shared);

  YACLIB_ASSERT(core);
  static constexpr auto From = Unique ? CoreType::FromUnique : CoreType::FromShared;
  auto* callback = MakeCore<CoreT | From, Arg, E>(std::forward<Func>(f));
  // TODO(MBkkt) callback, executor, caller should be in ctor
  if constexpr (IsDetach(CoreT)) {
    callback->StoreCallback(MakeDrop());
  }
  callback->_executor = executor;

  auto* caller = [&] {
    if constexpr (Unique) {
      return core.Release();
    } else {
      return core.Get();
    }
  }();

  if constexpr (!IsLazy(CoreT)) {
    Loop(caller, caller->template SetInline<false>(*callback));
  }

  using ResultCoreT = typename std::remove_reference_t<decltype(*callback)>::Base;
  if constexpr (IsLazy(CoreT)) {
    static_assert(!Shared, "Shared + Lazy (SharedTask) is not supported");
    callback->next = caller;
    caller->StoreCallback(*callback);
    return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
  } else if constexpr (!IsDetach(CoreT)) {
    // TODO(ocelaiwo): consider adding ThenShared etc. so add SharedFuture/On here
    if constexpr (On) {
      return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    } else {
      return Future{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    }
  }
}

}  // namespace yaclib::detail
