set (UBSAN 1)
if (NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  list(APPEND YACLIB_LINK_OPTIONS -fsanitize=undefined)
endif ()
list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=undefined,float-divide-by-zero)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=float-cast-overflow)
else ()
  list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=implicit-conversion,local-bounds)
endif ()
