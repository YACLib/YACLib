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

if (YACLIB_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${YACLIB_CXX_STANDARD})
elseif (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${YACLIB_SOURCE_DIR}/cmake")
include(yaclib_flags)

if (YACLIB_INSTALL)
  include(GNUInstallDirs)
  install(DIRECTORY ${YACLIB_SOURCE_DIR}/include/yaclib TYPE INCLUDE)
  install(DIRECTORY ${YACLIB_SOURCE_DIR}/include/yaclib_std TYPE INCLUDE)
  install(DIRECTORY ${YACLIB_BINARY_DIR}/include/yaclib TYPE INCLUDE)
endif()

add_subdirectory(src) # Create static library

if (YACLIB_TEST)
  enable_testing()
  add_subdirectory(test)
  message("CMAKE_CXX_COMPILER_ID : ${CMAKE_CXX_COMPILER_ID}")
  message("CMAKE_CXX_SIMULATE_ID : ${CMAKE_CXX_SIMULATE_ID}")
  message("YACLIB_LINK_OPTIONS   : ${YACLIB_LINK_OPTIONS}")
  message("YACLIB_COMPILE_OPTIONS: ${YACLIB_COMPILE_OPTIONS}")
  message("YACLIB_DEFINITIONS    : ${YACLIB_DEFINITIONS}")
endif ()
