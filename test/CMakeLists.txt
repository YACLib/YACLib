unset(CMAKE_INTERPROCEDURAL_OPTIMIZATION)

# GTest
find_package(GTest ${LEVEL_FIND_PACKAGE})

if (NOT GTEST_FOUND)
  fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  fetchcontent_makeavailable(googletest)
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
endif ()

# Google Benchmark
find_package(benchmark ${LEVEL_FIND_PACKAGE} QUIET)

if (NOT BENCHMARK_FOUND)
  set(BENCHMARK_ENABLE_TESTING NO)
  fetchcontent_declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.5.5
  )
  fetchcontent_makeavailable(googlebenchmark)
endif ()

if (ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif ()

add_compile_options(-Wall -Wextra -pedantic)

set(UTIL_HEADERS
  util/cpu_time.hpp
  util/time.hpp
  )

function(yaclib_test NAME)
  add_executable(${NAME}
    ${NAME}.cpp
    )
  target_sources(${NAME}
    PRIVATE ${UTIL_HEADERS}
    )
  target_link_libraries(${NAME}
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE benchmark::benchmark
    PRIVATE ${PROJECT_NAME}
    )
  target_include_directories(${NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    )
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

yaclib_test(task)
yaclib_test(thread_factory)
yaclib_test(thread_pool)
