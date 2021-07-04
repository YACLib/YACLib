cmake_minimum_required(VERSION 3.11)

# Include guards
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
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
if (ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Create static library
add_subdirectory(src)
target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PUBLIC ${CMAKE_BINARY_DIR} # for config.hpp
  PRIVATE ${CMAKE_SOURCE_DIR}/src
  )

if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()
