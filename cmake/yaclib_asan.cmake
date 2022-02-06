if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  list(APPEND YACLIB_COMPILE_OPTIONS
    /fsanitize=address
    /fsanitize-address-use-after-return
    )
else ()
  if (NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    list(APPEND YACLIB_LINK_OPTIONS -fsanitize=address)
    list(APPEND YACLIB_COMPILE_OPTIONS -fno-common)
  endif ()
  list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=address)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND YACLIB_COMPILE_OPTIONS
      -fsanitize=pointer-compare,pointer-subtract
      -fsanitize-address-use-after-scope
      )
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND YACLIB_COMPILE_OPTIONS
      -fsanitize=pointer-compare,pointer-subtract
      -fsanitize-address-use-after-scope
      -mllvm -asan-use-private-alias=1
      )
  endif ()
endif ()
