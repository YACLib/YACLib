# TODO(mkornaukhov03) add macOS support
# TODO(mkornaukhov03) need a more general approach

# Detect flags and coroutine support of compiler
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
    message(WARNING "Coroutines are only supported in GCC-10 or later")
  elseif (YACLIB_CXX_STANDARD VERSION_LESS 14)
    message(WARNING "Coroutines are supported in GCC with c++14 or later")
  else ()
    set(YACLIB_CORO 2)
    set(YACLIB_CORO_FLAGS "-fcoroutines")
  endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)
  if (NOT YACLIB_COMPILE_OPTIONS MATCHES "libc\\+\\+") # TODO(mkornaukhov03) how to rewrite better?
    message(WARNING "Clang + libstdc++ do not support support coroutines in a bundle")
  elseif (YACLIB_CXX_STANDARD VERSION_LESS 20)
    message(WARNING "Coroutines are supported in Clang with c++20 or later")
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0")
    message(WARNING "Coroutines are only supported in clang-9 or later")
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
    set(YACLIB_CORO 1)
  else ()
    set(YACLIB_CORO 2)
  endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  # TODO(MBkkt) /await:heapelide experimental but maybe useful for performance
  #  But I hear it works only for generators
  if (YACLIB_CXX_STANDARD VERSION_LESS 17)
    message("Coroutines are only supported in MSVC with c++17 or later")
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
    set(YACLIB_CORO 1)
    set(YACLIB_CORO_FLAGS "/await")
  else ()
    set(YACLIB_CORO 2)
    set(YACLIB_CORO_FLAGS "/await")
  endif ()
else ()
  message(WARNING "Coroutines are only supported in GCC/Clang/MSVC")
endif ()

list(APPEND YACLIB_COMPILE_OPTIONS "${YACLIB_CORO_FLAGS}")
list(APPEND YACLIB_LINK_OPTIONS "${YACLIB_CORO_FLAGS}")

