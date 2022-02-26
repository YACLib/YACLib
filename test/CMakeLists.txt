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
  util/cpu_time.hpp
  util/error_suite.hpp
  util/inline_helper.hpp
  util/error_code.hpp
  util/time.hpp
  )

set(YACLIB_EXAMPLE_SOURCES
  example/async/simple
  example/executor/thread_pool
  example/executor/strand
  example/async/when_all
  example/async/when_any
  )
set(YACLIB_UNIT_SOURCES
  unit/util/intrusive_ptr
  unit/util/result
  unit/executor/task
  unit/async/get
  unit/async/future
  unit/async/future_inline
  unit/async/future_functor
  unit/algo/when_all
  unit/algo/wait
  unit/executor/thread_factory
  unit/algo/when_any
  unit/algo/wait_group
  unit/executor/thread_pool
  unit/async/stress
  unit/executor/strand
  )

if (CORO)
  list(APPEND YACLIB_UNIT_SOURCES
    unit/coroutine/await
    unit/coroutine/dummy
    unit/coroutine/future_coro_traits
    unit/coroutine/stress
    )
endif ()

#TODO(MBkkt) Add options which shuffle tests list, maybe just use gtest options?
set(YACLIB_TEST_SOURCES ${YACLIB_UNIT_SOURCES} ${YACLIB_EXAMPLE_SOURCES})

function(yaclib_add_test TEST_NAME)
  target_compile_options(${TEST_NAME} PRIVATE ${YACLIB_WARN})
  target_compile_definitions(${TEST_NAME} PRIVATE ${YACLIB_DEFINITIONS})
  target_sources(${TEST_NAME}
    PRIVATE ${TEST_UTIL_INCLUDES}
    )
  target_link_libraries(${TEST_NAME}
    PRIVATE GTest::gtest
    PRIVATE ${PROJECT_NAME}
    )
  target_include_directories(${TEST_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${YACLIB_SOURCE_DIR}/src
    )
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

if (WARN IN_LIST YACLIB_FLAGS)
  include(yaclib_warn_test)
  message("test warnings: ${YACLIB_WARN}")
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
