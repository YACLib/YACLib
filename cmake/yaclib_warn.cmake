if (MSVC)
  #TODO(MBkkt) We have to use /Wall. But MSVC is dumb and uses this flag for external projects, like gtest.
  list(APPEND YACLIB_COMPILE_OPTIONS
    /W3
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  list(APPEND YACLIB_COMPILE_OPTIONS
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-ctad-maybe-unsupported
    -Wno-padded
    -Wno-exit-time-destructors
    -Wno-undefined-func-template # Needed for some hacks that speed up compilation
    -Wno-global-constructors   # TODO(MBkkt) Maybe fix it: Needed only for tests?
    -Wno-weak-template-vtables # TODO(MBkkt) Maybe fix it
    -Wno-switch-enum           # TODO(MBkkt) Maybe fix it
    -Wno-sign-conversion       # TODO(MBkkt) Maybe fix it
    -Wno-weak-vtables                       # TODO(MBkkt) Fix it
    -Wno-gnu-zero-variadic-macro-arguments  # TODO(MBkkt) Fix it
    -Wno-covered-switch-default  # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
    -Wno-shadow-uncaptured-local # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
    )
else ()
  list(APPEND YACLIB_COMPILE_OPTIONS
    -Wall
    -Wextra
    -pedantic
    )
endif ()
