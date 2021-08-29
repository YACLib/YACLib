cmake_minimum_required(VERSION 3.11)

# Include guards
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif ()

project(yaclib)

# Set variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()

if (MSVC)
  list(APPEND YACLIB_COMPILE_OPTIONS /W3)
  #TODO(MBkkt) We have to use /Wall. But MSVC is dumb and uses this flag for external projects, like gtest.
  if (SANITIZER)
    message("Sanitizer's work only on Linux")
  endif ()
else ()
  list(APPEND YACLIB_COMPILE_OPTIONS -Wall -Wextra -pedantic)
  if (SANITIZER STREQUAL ASAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=address)
    set(SANITIZER_COMPILE_OPTIONS
      # -fsanitize-address-use-after-return=runtime
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope
      )
  elseif (SANITIZER STREQUAL TSAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=thread)
    set(SANITIZER_COMPILE_OPTIONS
      # -fsanitize-address-use-after-return=runtime
      -gcolumn-info
      )
  elseif (SANITIZER STREQUAL UBSAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=undefined)
  elseif (SANITIZER STREQUAL MEMSAN) # Memory Santizer doesn't work with GTest (https://github.com/google/sanitizers/issues/648)
    set(SANITIZER_ENABLE_OPTION -fsanitize=memory)
  endif ()
  list(APPEND YACLIB_COMPILE_OPTIONS
    -fno-omit-frame-pointer
    ${SANITIZER_ENABLE_OPTION}
    ${SANITIZER_COMPILE_OPTIONS}
    )
  list(APPEND YACLIB_LINK_OPTIONS ${SANITIZER_ENABLE_OPTION})
endif ()
if (COVERAGE AND CMAKE_SYSTEM_NAME STREQUAL Linux)
  list(APPEND YACLIB_COMPILE_OPTIONS
    --coverage
    )
  list(APPEND YACLIB_LINK_OPTIONS
    --coverage
    )
endif ()
message("Build options: ${YACLIB_COMPILE_OPTIONS}")
message("Link  options: ${YACLIB_LINK_OPTIONS}")

include(FetchContent)

# Create static library
add_subdirectory(src)

if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()
