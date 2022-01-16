find_package(GTest QUIET)
find_package(benchmark QUIET)

if (NOT GTEST_FOUND OR NOT BENCHMARK_FOUND)
  include(FetchContent)
  if (NOT GTEST_FOUND)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.11.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
    FetchContent_MakeAvailable(googletest)
    set(GTEST_BOTH_LIBRARIES GTest::gtest)
  endif ()
  if (NOT BENCHMARK_FOUND)
    set(BENCHMARK_ENABLE_TESTING NO)
    FetchContent_Declare(
      benchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.5.5
    )
    FetchContent_MakeAvailable(benchmark)
  endif ()
endif ()

add_compile_options(${YACLIB_COMPILE_OPTIONS})
add_link_options(${YACLIB_LINK_OPTIONS})

set(TEST_UTIL_INCLUDES
  util/cpu_time.hpp
  util/time.hpp
  main.cpp
  )
set(YACLIB_EXAMPLE_SOURCES)
set(YACLIB_EXAMPLE_INCLUDES)

#TODO(myannyax) any ways this can be avoided?
if (YACLIB_FAULT)
  add_compile_definitions(YACLIB_FAULT)
endif ()

if (YACLIB_ASSERT_FATAL)
  add_compile_definitions(YACLIB_ASSERT_FATAL)
elseif (YACLIB_ASSERT_ERROR)
  add_compile_definitions(YACLIB_ASSERT_IGNORE)
elseif (YACLIB_ASSERT_IGNORE)
  add_compile_definitions(YACLIB_ASSERT_IGNORE)
endif ()

set(YACLIB_UNIT_SOURCES
  unit/util/intrusive_ptr
  unit/util/result
  unit/executor/task
  unit/async/get
  unit/async/future
  unit/algo/when_all
  unit/algo/wait
  unit/executor/thread_factory
  unit/algo/when_any
  unit/algo/wait_group
  unit/executor/thread_pool
  unit/async/stress
  unit/executor/strand
  )

if (YACLIB_BUILD_EXAMPLES)
  set(YACLIB_EXAMPLE_SOURCES
    example/async/simple
    example/executor/thread_pool
    example/executor/strand
    example/async/when_all
    example/async/when_any
    )
endif ()

#TODO(MBkkt) Add options which shuffle tests list
set(YACLIB_TEST_SOURCES ${YACLIB_UNIT_SOURCES} ${YACLIB_EXAMPLE_SOURCES})

foreach (TEST_SOURCE ${YACLIB_TEST_SOURCES})
  string(REPLACE "/" "_" TEST_NAME ${TEST_SOURCE})
  add_executable(${TEST_NAME} ${TEST_SOURCE}.cpp)
  target_sources(${TEST_NAME}
    PRIVATE ${TEST_UTIL_INCLUDES}
    )
  target_link_libraries(${TEST_NAME}
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE benchmark::benchmark benchmark::benchmark_main
    PRIVATE ${PROJECT_NAME}
    )
  target_include_directories(${TEST_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${YACLIB_SOURCE_DIR}/src
    )
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach ()
