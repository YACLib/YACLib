list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/async/run.hpp
  ${YACLIB_INCLUDE_DIR}/async/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/promise.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/async/detail/core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/future_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/promise_impl.hpp
  )

add_files()
