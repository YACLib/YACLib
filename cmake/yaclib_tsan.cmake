list(APPEND YACLIB_LINK_OPTIONS -fsanitize=thread)
list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=thread)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  list(APPEND YACLIB_COMPILE_OPTIONS -gcolumn-info)
endif ()
