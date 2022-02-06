if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # TODO(MBkkt) We have to use /Wall. We need to disable some paranoid warnings, but I don't have windows.
  list(APPEND YACLIB_WARN
    /W3
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND YACLIB_WARN
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-ctad-maybe-unsupported
    -Wno-padded
    -Wno-exit-time-destructors
    -Wno-undefined-func-template # Needed for some hacks that speed up compilation
    -Wno-weak-template-vtables   # TODO(MBkkt) Maybe fix it
    -Wno-switch-enum             # TODO(MBkkt) Maybe fix it
    -Wno-weak-vtables            # TODO(MBkkt) Fix it
    -Wno-global-constructors   # TODO(MBkkt) Maybe fix it: Needed only for tests
    -Wno-sign-conversion       # TODO(MBkkt) Maybe fix it: Needed only for tests
    -Wno-gnu-zero-variadic-macro-arguments  # TODO(MBkkt) Fix it: Needed only for tests
    -Wno-covered-switch-default  # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
    -Wno-shadow-uncaptured-local # TODO(MBkkt) Fix it: test/unit/algo/when_any.cpp
    )
  if (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    list(APPEND YACLIB_WARN
      -Wno-undef # For gtest
      )
  endif ()
else ()
  list(APPEND YACLIB_WARN
    -Wall
    -Wextra
    -pedantic
    )
endif ()
