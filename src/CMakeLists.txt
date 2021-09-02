macro(add_files)
  set(YACLIB_INCLUDES ${YACLIB_INCLUDES} PARENT_SCOPE)
  set(YACLIB_HEADERS ${YACLIB_HEADERS} PARENT_SCOPE)
  set(YACLIB_SOURCES ${YACLIB_SOURCES} PARENT_SCOPE)
endmacro()

# yaclib library
add_compile_options(${YACLIB_COMPILE_OPTIONS})
add_link_options(${YACLIB_LINK_OPTIONS})

configure_file(config.hpp.in ${YACLIB_BINARY_DIR}/include/yaclib/config.hpp)

set(YACLIB_INCLUDE_DIR ${YACLIB_SOURCE_DIR}/include/yaclib)

set(
  YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/ref.hpp
  ${YACLIB_INCLUDE_DIR}/task.hpp
  ${YACLIB_INCLUDE_DIR}/async/run.hpp
  ${YACLIB_INCLUDE_DIR}/util/counters.hpp
  ${YACLIB_INCLUDE_DIR}/util/result.hpp
  ${YACLIB_INCLUDE_DIR}/util/type_traits.hpp
)
set(
  YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/async/detail/core.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/future.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/promise.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/future_impl.hpp
  ${YACLIB_INCLUDE_DIR}/async/detail/promise_impl.hpp
)

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
  PUBLIC ${YACLIB_SOURCE_DIR}/include
  PUBLIC ${YACLIB_BINARY_DIR}/include # for config.hpp
  PRIVATE ${YACLIB_SOURCE_DIR}/src
)
