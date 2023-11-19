list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/coro/await.hpp
  ${YACLIB_INCLUDE_DIR}/coro/await_on.hpp
  ${YACLIB_INCLUDE_DIR}/coro/coro.hpp
  ${YACLIB_INCLUDE_DIR}/coro/current_executor.hpp
  ${YACLIB_INCLUDE_DIR}/coro/future.hpp
  ${YACLIB_INCLUDE_DIR}/coro/guard_sticky.hpp
  ${YACLIB_INCLUDE_DIR}/coro/guard.hpp
  ${YACLIB_INCLUDE_DIR}/coro/mutex.hpp
  ${YACLIB_INCLUDE_DIR}/coro/on.hpp
  ${YACLIB_INCLUDE_DIR}/coro/task.hpp
  ${YACLIB_INCLUDE_DIR}/coro/yield.hpp
  )

list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/coro/detail/await_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coro/detail/await_on_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coro/detail/on_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coro/detail/promise_type.hpp
  )

add_files()
