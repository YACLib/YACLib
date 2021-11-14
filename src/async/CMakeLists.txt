list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/async/run.hpp
  ${YACLIB_INCLUDE_DIR}/async/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/promise.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/async/detail/base_core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/future_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/promise_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/result_core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/wait_core.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/base_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/future_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/promise_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/result_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wait_core.cpp
  )

add_files()
