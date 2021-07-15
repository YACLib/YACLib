find_package(GTest ${LEVEL_FIND_PACKAGE})

if (NOT GTEST_FOUND)
  unset(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  fetchcontent_makeavailable(googletest)
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  if (ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else ()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  endif ()
endif ()

add_compile_options(-Wall -Wextra -pedantic)

function(yaclib_test NAME)
  add_executable(${NAME} ${NAME}.cpp)
  target_link_libraries(${NAME}
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE ${PROJECT_NAME}
    )
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

yaclib_test(task)
yaclib_test(thread_factory)
