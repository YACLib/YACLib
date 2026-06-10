cmake_minimum_required(VERSION 3.13)

if (YACLIB_LINK_OPTIONS OR YACLIB_COMPILE_OPTIONS)
  add_link_options(${YACLIB_LINK_OPTIONS})
  add_compile_options(${YACLIB_COMPILE_OPTIONS})
else ()
  find_package(GTest QUIET)
endif ()

if (NOT GTEST_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
    )
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE) # TODO(MBkkt) We can install and reuse in CI?
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)   # May be enabled later
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif ()

set(TEST_UTIL_INCLUDES
  util/async_suite.hpp
  util/cpu_time.hpp
  util/error_code.hpp
  util/error_suite.hpp
  util/helpers.hpp
  util/inline_helper.hpp
  util/time.hpp
  util/when_suite.hpp
  )
set(TEST_UTIL_SOURCES
  test.cpp
  )

set(YACLIB_EXAMPLE_SOURCES
  example/async/simple
  example/exe/thread_pool
  example/exe/strand
  example/async/when_all
  example/async/when_any
  )
set(YACLIB_UNIT_SOURCES
  unit/async/connect
  unit/async/core_size
  unit/util/intrusive_ptr
  unit/util/result
  unit/exe/task
  unit/async/task
  unit/async/make_task
  unit/async/make_future
  unit/async/get
  unit/async/future
  unit/async/future_inline
  unit/async/future_functor
  unit/algo/join
  unit/algo/when
  unit/algo/when_all
  unit/algo/when_all_tuple
  unit/algo/wait
  unit/algo/when_any
  unit/algo/wait_group
  unit/runtime/fair_thread_pool
  unit/async/shared_future
  unit/async/stress
  unit/exe/strand
  unit/not_implemented
  )

if (YACLIB_CORO_NEED)
  list(APPEND YACLIB_UNIT_SOURCES
    unit/coro/await
    unit/coro/await_group
    unit/coro/await_group_stress
    unit/coro/async_mutex
    unit/coro/async_mutex_stress
    unit/coro/async_shared_mutex
    unit/coro/future_coro_traits
    unit/coro/stress
    unit/coro/on
    )
endif ()

if (YACLIB_FAULT STREQUAL "FIBER")
  list(APPEND YACLIB_UNIT_SOURCES
    unit/fault/mutexes
    unit/fault/thread_local
    )
endif ()

#TODO(MBkkt) Add options which shuffle tests list, maybe just use gtest options?
set(YACLIB_TEST_SOURCES ${YACLIB_UNIT_SOURCES} ${YACLIB_EXAMPLE_SOURCES})

set(OLD_YACLIB_DEFINITIONS YACLIB_DEFINITIONS)

if (NOT YACLIB_DEFINITIONS MATCHES "YACLIB_CI_SLOWDOWN")
  list(APPEND YACLIB_DEFINITIONS "YACLIB_CI_SLOWDOWN=1")
endif ()

function(yaclib_add_test TEST_NAME)
  target_compile_options(${TEST_NAME} PRIVATE ${YACLIB_WARN})
  target_compile_definitions(${TEST_NAME} PRIVATE ${YACLIB_DEFINITIONS})
  target_sources(${TEST_NAME}
    PRIVATE ${TEST_UTIL_INCLUDES}
    )
  target_link_libraries(${TEST_NAME}
    PRIVATE GTest::gtest
    PRIVATE yaclib
    )
  target_include_directories(${TEST_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${YACLIB_SOURCE_DIR}/src
    )
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

if (WARN IN_LIST YACLIB_FLAGS)
  include(yaclib_warn_test)
  message("YACLib/test warnings: ${YACLIB_WARN}")
endif ()

if (YACLIB_TEST STREQUAL "SINGLE")
  list(TRANSFORM YACLIB_TEST_SOURCES APPEND .cpp)
  add_executable(yaclib_test test.cpp ${YACLIB_TEST_SOURCES})
  yaclib_add_test(yaclib_test)
else ()
  foreach (TEST_SOURCE ${YACLIB_TEST_SOURCES})
    string(REPLACE "/" "_" TEST_NAME ${TEST_SOURCE})
    add_executable(${TEST_NAME} test.cpp ${TEST_SOURCE}.cpp)
    yaclib_add_test(${TEST_NAME})
  endforeach ()
endif ()

unset(YACLIB_TEST_SOURCES)

if (NOT YACLIB_SAN AND YACLIB_CORO_NEED)
  add_executable(yaclib_dealloc_order_test test.cpp unit/async/dealloc_order.cpp)
  yaclib_add_test(yaclib_dealloc_order_test)
endif ()

if (NOT "YACLIB_LOG_DEBUG" IN_LIST YACLIB_DEFINITIONS)
  list(APPEND YACLIB_DEFINITIONS "YACLIB_LOG_DEBUG")
endif ()
if (NOT "YACLIB_LOG_WARN" IN_LIST YACLIB_DEFINITIONS)
  list(APPEND YACLIB_DEFINITIONS "YACLIB_LOG_WARN")
endif ()

add_executable(yaclib_log_test test.cpp unit/log.cpp)
yaclib_add_test(yaclib_log_test)

# yaclib_ext tests: integrations that require specific libraries or standards.
# YACLIB_EXT_TEST=OFF skips them (e.g. sanitizer/coverage jobs that target yaclib itself)
if (NOT DEFINED YACLIB_EXT_TEST)
  set(YACLIB_EXT_TEST AUTO)
endif ()

if (NOT YACLIB_EXT_TEST STREQUAL "OFF")
  # Real compile probe: the cxx_std_23 feature flag is not enough, old clang with new
  # libstdc++ advertises it but cannot parse the C++23 headers
  if (cxx_std_23 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/probe_expected.cpp
      "#include <expected>\n#include <ranges>\n#include <vector>\n"
      "int main() {\n"
      "  std::vector<int> v{1};\n"
      "  for (auto x : v | std::views::transform([](int y) { return y; })) { (void)x; }\n"
      "  return std::expected<int, int>{0}.value();\n"
      "}\n")
    try_compile(YACLIB_HAS_STD_EXPECTED
      ${CMAKE_CURRENT_BINARY_DIR}/probe_expected
      ${CMAKE_CURRENT_BINARY_DIR}/probe_expected.cpp
      CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
  endif ()
  if (YACLIB_HAS_STD_EXPECTED)
    add_executable(yaclib_ext_std_test test.cpp ext/std/expected.cpp)
    yaclib_add_test(yaclib_ext_std_test)
    set_target_properties(yaclib_ext_std_test PROPERTIES CXX_STANDARD 23)
  else ()
    message(STATUS "yaclib_ext_std_test skipped: no usable std::expected")
  endif ()

  # abseil master compiler floor is above yaclib's
  set(YACLIB_EXT_ABSL TRUE)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    set(YACLIB_EXT_ABSL FALSE)
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    set(YACLIB_EXT_ABSL FALSE)
  endif ()
  if (YACLIB_EXT_ABSL)
    find_package(absl QUIET)
    if (NOT absl_FOUND)
      include(FetchContent)
      FetchContent_Declare(abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG master
        )
      set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
      set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
      FetchContent_MakeAvailable(abseil)
    endif ()
    add_executable(yaclib_ext_absl_test test.cpp ext/absl/status_or.cpp)
    yaclib_add_test(yaclib_ext_absl_test)
    target_link_libraries(yaclib_ext_absl_test PRIVATE absl::status absl::statusor)
  else ()
    message(STATUS "yaclib_ext_absl_test skipped: compiler below abseil floor")
  endif ()
endif ()

set(YACLIB_DEFINITIONS OLD_YACLIB_DEFINITIONS)
