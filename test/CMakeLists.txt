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

if (ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()

add_compile_options(${YACLIB_COMPILE_OPTIONS})
add_link_options(${YACLIB_LINK_OPTIONS})

set(
  TEST_UTIL_HEADERS
  util/cpu_time.hpp
  util/time.hpp
)

function(yaclib_test NAME)
  add_executable(
    ${NAME}
    ${NAME}.cpp
  )
  target_sources(
    ${NAME}
    PRIVATE ${UTIL_HEADERS}
  )
  target_link_libraries(
    ${NAME}
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE benchmark::benchmark
    PRIVATE ${PROJECT_NAME}
  )
  target_include_directories(
    ${NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
  )
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

yaclib_test(task)
yaclib_test(thread_factory)
yaclib_test(thread_pool)
yaclib_test(async_mutex)
yaclib_test(intrusive_ptr)
