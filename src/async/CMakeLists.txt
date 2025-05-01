list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/async/contract.hpp
  ${YACLIB_INCLUDE_DIR}/async/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/make.hpp
  ${YACLIB_INCLUDE_DIR}/async/promise.hpp
  ${YACLIB_INCLUDE_DIR}/async/run.hpp
  ${YACLIB_INCLUDE_DIR}/async/wait.hpp
  ${YACLIB_INCLUDE_DIR}/async/wait_for.hpp
  ${YACLIB_INCLUDE_DIR}/async/wait_until.hpp
  ${YACLIB_INCLUDE_DIR}/async/when_all.hpp
  ${YACLIB_INCLUDE_DIR}/async/when_any.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/async/detail/wait_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/when_all_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/when_any_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/when_impl.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/future_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/promise_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wait_impl.cpp
  )

add_files()
