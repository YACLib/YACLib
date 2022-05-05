list(APPEND YACLIB_INCLUDES
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

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/await_awaiter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/on.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/on_awaiter.cpp
  )

add_files()
