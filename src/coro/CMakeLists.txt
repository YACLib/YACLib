list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/coro/async_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/coro/await.hpp
  ${YACLIB_INCLUDE_DIR}/coro/coroutine.hpp
  ${YACLIB_INCLUDE_DIR}/coro/future_traits.hpp
  ${YACLIB_INCLUDE_DIR}/coro/on.hpp
  )

list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/coro/detail/await_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coro/detail/promise_type.hpp
  ${YACLIB_INCLUDE_DIR}/coro/detail/on_awaiter.hpp
  )

add_files()
