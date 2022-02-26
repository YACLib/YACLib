list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/coroutine.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/future_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/promise_type.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/resume_deleter.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/suspend_condition.hpp
)

list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/coroutine/await.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/future_coro_traits.hpp
)
add_files()
