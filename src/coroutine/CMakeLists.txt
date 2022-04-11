list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/coroutine/await.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/coroutine.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/future_traits.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/via.hpp
  )

list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/await_awaiter.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/promise_type.hpp
  ${YACLIB_INCLUDE_DIR}/coroutine/detail/via_awaiter.hpp
  )

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/await_awaiter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/via.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/via_awaiter.cpp
  )

add_files()
