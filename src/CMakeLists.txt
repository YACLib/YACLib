macro(add_files)
  set(YACLIB_INCLUDES ${YACLIB_INCLUDES} PARENT_SCOPE)
  set(YACLIB_HEADERS  ${YACLIB_HEADERS}  PARENT_SCOPE)
  set(YACLIB_SOURCES  ${YACLIB_SOURCES}  PARENT_SCOPE)
endmacro()

# yaclib library
add_compile_options(${YACLIB_COMPILE_OPTIONS})
add_link_options(${YACLIB_LINK_OPTIONS})

configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/include/yaclib/config.hpp)

set(YACLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/yaclib)

set(
  YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/ref.hpp
  ${YACLIB_INCLUDE_DIR}/task.hpp
  ${YACLIB_INCLUDE_DIR}/async/core.hpp
  ${YACLIB_INCLUDE_DIR}/async/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/promise.hpp
  ${YACLIB_INCLUDE_DIR}/async/async.hpp
)
set(YACLIB_HEADERS)
set(YACLIB_SOURCES)

add_subdirectory(container)
add_subdirectory(executor)

add_library(
  ${PROJECT_NAME} STATIC
  ${YACLIB_SOURCES}
)
target_sources(
  ${PROJECT_NAME}
  PUBLIC ${YACLIB_INCLUDES}
  PRIVATE ${YACLIB_HEADERS}
)

find_package(Threads REQUIRED)
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC Threads::Threads
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PUBLIC ${CMAKE_BINARY_DIR}/include # for config.hpp
  PRIVATE ${CMAKE_SOURCE_DIR}/src
)
