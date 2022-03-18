# TODO(mkornaukhov03) need a more general approach 
# TODO(mkornaukhov03) add macos support

set(YACLIB_CORO_FINAL 0)
set(YACLIB_CORO_EXPERIMENTAL 0)
set(YACLIB_CORO_FLAGS "")

# Detect flags and coroutine support of compiler
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
    message("Coroutines are only supported in GCC-10 or later")
    set(CORO OFF)
  elseif (CMAKE_CXX_STANDARD VERSION_LESS 14)
    message("Coroutines are supported in GCC with c++14 or later")
    set(CORO OFF)
  else ()
    set(YACLIB_CORO_FINAL 1)
    set(YACLIB_CORO_FLAGS "-fcoroutines")
  endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)
  if (NOT YACLIB_COMPILE_OPTIONS MATCHES "-stdlib=libc") #TODO(mkornaukhov03) how to rewrite better?
    message("Clang + libstdc++ do not support support coroutines in a bundle")
    set(CORO OFF)
  elseif (CMAKE_CXX_STANDARD VERSION_LESS 20)
    message("Coroutines are supported in Clang with c++20 or later")
    set(CORO OFF)
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0") # TODO(mkornaukhov03) or higher?
    message("Coroutines are only supported in clang-9 or later")
    set(CORO OFF)
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
    set(YACLIB_CORO_EXPERIMENTAL 1)
  else ()
    set(YACLIB_CORO_FINAL 1)
  endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  if (CMAKE_CXX_STANDARD VERSION_LESS 17)
    message("Coroutines are only supported in MSVC with c++17 or later")
    set(CORO OFF)
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
    set(YACLIB_CORO_EXPERIMENTAL 1)
    set(YACLIB_CORO_FLAGS "/await")
  else ()
    set(YACLIB_CORO_FINAL 1)
    set(YACLIB_CORO_FLAGS "/await")
  endif ()
else ()
  set(CORO OFF)
  message("Coroutines are only supported in GCC/Clang/MSVC")
endif ()

list(APPEND YACLIB_COMPILE_OPTIONS "${YACLIB_CORO_FLAGS}")
list(APPEND YACLIB_LINK_OPTIONS "${YACLIB_CORO_FLAGS}")

# Configure file
configure_file("cmake/config_coro.hpp.in" "${YACLIB_BINARY_DIR}/include/yaclib/coro_config.hpp")


#if (NOT YACLIB_CXX_STANDARD STREQUAL 20)
#  message("Error: coroutines are supported only with C++20!")
#  set (CORO OFF)
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
#  list(APPEND YACLIB_COMPILE_OPTIONS -fcoroutines)
#  list(APPEND YACLIB_LINK_OPTIONS -fcoroutines)
#endif()


