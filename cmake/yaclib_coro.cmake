set(YACLIB_CORO_NEED 1)
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(YACLIB_CORO_FLAGS "-fcoroutines")
elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  # TODO(MBkkt) /await:heapelide experimental but maybe useful for performance
  #  But I hear it works only for generators
  if (MSVC_VERSION GREATER 1928)
    set(YACLIB_CORO_FLAGS "/await:strict")
  else ()
    set(YACLIB_CORO_FLAGS "/await")
  endif ()
endif ()

list(APPEND YACLIB_COMPILE_OPTIONS "${YACLIB_CORO_FLAGS}")
