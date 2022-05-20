if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(YACLIB_CORO_FLAGS "-fcoroutines")
elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  # TODO(MBkkt) /await:heapelide experimental but maybe useful for performance
  #  But I hear it works only for generators
  set(YACLIB_CORO_FLAGS "/await")
endif ()
set(YACLIB_CORO 1)
list(APPEND YACLIB_COMPILE_OPTIONS "${YACLIB_CORO_FLAGS}")
list(APPEND YACLIB_LINK_OPTIONS "${YACLIB_CORO_FLAGS}")

