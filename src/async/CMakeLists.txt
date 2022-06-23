list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/async/contract.hpp
  ${YACLIB_INCLUDE_DIR}/async/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/future_splitter.hpp
  ${YACLIB_INCLUDE_DIR}/async/promise.hpp
  ${YACLIB_INCLUDE_DIR}/async/run.hpp
  ${YACLIB_INCLUDE_DIR}/async/shared_promise.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/async/detail/base_core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/future_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/inline_core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/promise_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/result_core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/shared_promise_impl.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/base_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/future_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/promise_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/result_core.cpp
  )

add_files()
