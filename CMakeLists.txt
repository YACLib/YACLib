cmake_minimum_required(VERSION 3.11)

project(yaclib)

set(YACLIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(YACLIB_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Include guards
if (YACLIB_SOURCE_DIR STREQUAL YACLIB_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif ()

if (NOT YACLIB_CXX_STANDARD)
  set(YACLIB_CXX_STANDARD 17)
endif ()

# Set variables
set(CMAKE_CXX_STANDARD ${YACLIB_CXX_STANDARD})
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
if (YACLIB_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()

if (MSVC)
  list(APPEND YACLIB_COMPILE_OPTIONS /W3)
  #TODO(MBkkt) We have to use /Wall. But MSVC is dumb and uses this flag for external projects, like gtest.
  if (YACLIB_SANITIZER)
    message("Sanitizer's work only on Linux")
  endif ()
else ()
  list(APPEND YACLIB_COMPILE_OPTIONS -Wall -Wextra -pedantic)
  if (YACLIB_SANITIZER STREQUAL ASAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=address)
    set(SANITIZER_COMPILE_OPTIONS
      # -fsanitize-address-use-after-return=runtime
      -fsanitize-recover=address
      -fsanitize-address-use-after-scope
      )
  elseif (YACLIB_SANITIZER STREQUAL TSAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=thread)
    set(SANITIZER_COMPILE_OPTIONS
      -gcolumn-info
      )
  elseif (YACLIB_SANITIZER STREQUAL UBSAN)
    set(SANITIZER_ENABLE_OPTION -fsanitize=undefined)
  elseif (YACLIB_SANITIZER STREQUAL MEMSAN) # Memory Santizer doesn't work with GTest (https://github.com/google/sanitizers/issues/648)
    set(SANITIZER_ENABLE_OPTION -fsanitize=memory)
  endif ()
  list(APPEND YACLIB_COMPILE_OPTIONS
    ${SANITIZER_ENABLE_OPTION}
    ${SANITIZER_COMPILE_OPTIONS}
    )
  list(APPEND YACLIB_LINK_OPTIONS ${SANITIZER_ENABLE_OPTION})
endif ()
if (YACLIB_COVERAGE AND CMAKE_SYSTEM_NAME STREQUAL Linux)
  list(APPEND YACLIB_COMPILE_OPTIONS
    --coverage
    )
  list(APPEND YACLIB_LINK_OPTIONS
    --coverage
    )
endif ()
message("YACLIB_COMPILE_OPTIONS: ${YACLIB_COMPILE_OPTIONS}")
message("YACLIB_LINK_OPTIONS   : ${YACLIB_LINK_OPTIONS}")
add_definitions(${YACLIB_DEFINITIONS})

# Create static library
add_subdirectory(src)

if (YACLIB_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()
