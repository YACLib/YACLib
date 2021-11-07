unset(CMAKE_INTERPROCEDURAL_OPTIMIZATION)

# GTest
find_package(GTest QUIET)

if (NOT GTEST_FOUND)
  fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
  fetchcontent_makeavailable(googletest)
  set(GTEST_BOTH_LIBRARIES GTest::gtest GTest::gtest_main)
endif ()

# Google Benchmark
find_package(benchmark QUIET)

if (NOT BENCHMARK_FOUND)
  set(BENCHMARK_ENABLE_TESTING NO)
  fetchcontent_declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.5.5
  )
  fetchcontent_makeavailable(benchmark)
endif ()

if (YACLIB_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()

add_compile_options(${YACLIB_COMPILE_OPTIONS})
add_link_options(${YACLIB_LINK_OPTIONS})

set(TEST_UTIL_INCLUDES
  util/cpu_time.hpp
  util/time.hpp
  )
set(YACLIB_EXAMPLE_SOURCES)
set(YACLIB_EXAMPLE_INCLUDES)

#TODO(MBkkt) Add options which shuffle tests list
set(YACLIB_UNITS
  unit/util/intrusive_ptr
  unit/util/result
  unit/executor/task
  unit/async/future
  unit/algo/when_all
  unit/algo/wait
  unit/algo/when_any
  unit/executor/thread_factory
  unit/executor/thread_pool
  unit/executor/serial
  unit/fiber/basic
  )

if (YACLIB_BUILD_EXAMPLES)
  set(YACLIB_EXAMPLES
    example/async/simple
    example/executor/thread_pool
    example/executor/serial
    example/async/when_all
    example/async/when_any
    )
endif ()

set(YACLIB_TEST_SOURCES ${YACLIB_UNITS} ${YACLIB_EXAMPLES})
foreach (TEST_SOURCE ${YACLIB_TEST_SOURCES})
  string(REPLACE "/" "_" TEST_NAME ${TEST_SOURCE})
  add_executable(${TEST_NAME} ${TEST_SOURCE}.cpp)
  target_sources(${TEST_NAME}
    PRIVATE ${TEST_UTIL_INCLUDES}
    )
  target_link_libraries(${TEST_NAME}
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE benchmark::benchmark
    PRIVATE ${PROJECT_NAME}
    )
  target_include_directories(${TEST_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${YACLIB_SOURCE_DIR}/src
    )
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach ()
