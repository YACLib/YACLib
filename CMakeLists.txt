cmake_minimum_required(VERSION 3.3)

project(yaclib
  LANGUAGES CXX
  )

set(YACLIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(YACLIB_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Include guards
if (YACLIB_SOURCE_DIR STREQUAL YACLIB_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif ()

if (NOT YACLIB_CXX_STANDARD)
  set(YACLIB_CXX_STANDARD 17)
endif ()

if (NOT YACLIB_C_STANDARD)
  set(YACLIB_C_STANDARD 11)
endif ()

# Set variables
set(CMAKE_CXX_STANDARD ${YACLIB_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_C_STANDARD ${YACLIB_C_STANDARD})
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (YACLIB_WARNING)
  if (MSVC)
    #TODO(MBkkt) We have to use /Wall. But MSVC is dumb and uses this flag for external projects, like gtest.
    list(APPEND YACLIB_COMPILE_OPTIONS
      /W3
      )
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    list(APPEND YACLIB_COMPILE_OPTIONS
      -Weverything
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-ctad-maybe-unsupported
      -Wno-padded
      -Wno-exit-time-destructors
      -Wno-undefined-func-template # Needed for some hacks that speed up compilation
      -Wno-global-constructors   # TODO(MBkkt) Maybe fix it: Needed only for tests?
      -Wno-weak-template-vtables # TODO(MBkkt) Maybe fix it
      -Wno-switch-enum           # TODO(MBkkt) Maybe fix it
      -Wno-sign-conversion       # TODO(MBkkt) Maybe fix it
      -Wno-weak-vtables                       # TODO(MBkkt) Fix it
      -Wno-gnu-zero-variadic-macro-arguments  # TODO(MBkkt) Fix it
      -Wno-covered-switch-default  # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
      -Wno-shadow-uncaptured-local # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
      )
  else ()
    list(APPEND YACLIB_COMPILE_OPTIONS
      -Wall
      -Wextra
      -pedantic
      )
  endif ()
endif ()

if (YACLIB_SANITIZER STREQUAL ASAN)
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=address)
  list(APPEND YACLIB_COMPILE_OPTIONS
    -fsanitize=address,pointer-compare,pointer-subtract
    -fsanitize-address-use-after-scope
    -fno-common
    -mllvm -asan-use-private-alias=1
    )
elseif (YACLIB_SANITIZER STREQUAL TSAN)
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=thread)
  list(APPEND YACLIB_COMPILE_OPTIONS
    -fsanitize=thread
    # -gcolumn-info
    )
elseif (YACLIB_SANITIZER STREQUAL UBSAN)
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=undefined)
  list(APPEND YACLIB_COMPILE_OPTIONS
    -fsanitize=undefined,float-divide-by-zero,implicit-conversion,local-bounds
    )
elseif (YACLIB_SANITIZER STREQUAL LSAN)
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=leak)
  list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=leak)
elseif (YACLIB_SANITIZER STREQUAL MEMSAN)
  # Memory Sanitizer doesn't work without compile stdlib with it
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=memory)
  list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=memory)
endif ()
if (YACLIB_SANITIZER) # Nicer stack trace
  list(APPEND YACLIB_COMPILE_OPTIONS -fno-omit-frame-pointer -fsanitize-recover=all)
endif ()

if (YACLIB_COVERAGE)
  list(APPEND YACLIB_COMPILE_OPTIONS
    --coverage
    )
  list(APPEND YACLIB_LINK_OPTIONS
    --coverage
    )
endif ()

add_definitions(${YACLIB_DEFINITIONS})

message("YACLIB_COMPILE_OPTIONS: ${YACLIB_COMPILE_OPTIONS}")
message("YACLIB_LINK_OPTIONS   : ${YACLIB_LINK_OPTIONS}")
message("YACLIB_DEFINITIONS    : ${YACLIB_DEFINITIONS}")

# Create static library
add_subdirectory(src)

if (YACLIB_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()
