if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # TODO(MBkkt) We have to use /Wall. But MSVC is dumb and uses this flag for external projects, like gtest.
  list(APPEND YACLIB_WARN
    /Wall
    /wd4625 # copy constructor was implicitly defined as deleted
    /wd4626 # assignment operator was implicitly defined as deleted
    /wd5026 # move constructor was implicitly defined as deleted
    /wd5027 # move assignment operator was implicitly defined as deleted
    /wd4514 # TODO(MBkkt) Fix it (unreferenced inline function has been removed)
    /wd4820 # TODO(MBkkt) Maybe ix it (bytes padding added after data member)
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?Clang")
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
    )
else ()
  list(APPEND YACLIB_WARN
    -Wall
    -Wextra
    -pedantic
    )
endif ()
