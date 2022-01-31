set(FIND YACLIB_FLAGS "COROUTINE" YACLIB_COROUTINE)
set(FIND YACLIB_FLAGS "" YACLIB_WARN)
set(FIND YACLIB_FLAGS "" YACLIB_ASAN)
set(FIND YACLIB_FLAGS "" YACLIB_UBSAN)
set(FIND YACLIB_FLAGS "" YACLIB_LSAN)
set(FIND YACLIB_FLAGS "" YACLIB_TSAN)
set(FIND YACLIB_FLAGS "" YACLIB_MEMSAN)
set(FIND YACLIB_FLAGS "COVERAGE" YACLIB_COVERAGE)

if (WARN IN_LIST YACLIB_FLAGS)
  include(yaclib_warn)
endif ()
set(SAN OFF)
if (ASAN IN_LIST YACLIB_FLAGS)
  set(SAN ON)
  include(yaclib_asan)
endif ()
if (UBSAN IN_LIST YACLIB_FLAGS)
  set(SAN ON)
  include(yaclib_ubsan)
endif ()
if (LSAN IN_LIST YACLIB_FLAGS)
  set(SAN ON)
  include(yaclib_lsan)
endif ()
if (TSAN IN_LIST YACLIB_FLAGS)
  set(SAN ON)
  include(yaclib_tsan)
endif ()
if (MEMSAN IN_LIST YACLIB_FLAGS)
  set(SAN ON)
  include(yaclib_memsan)
endif ()
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL MSVC AND SAN) # Nicer stack trace and recover
  list(APPEND YACLIB_COMPILE_OPTIONS -fno-omit-frame-pointer -fsanitize-recover=all)
endif ()

if (COVERAGE IN_LIST YACLIB_FLAGS)
  list(APPEND YACLIB_LINK_OPTIONS --coverage)
  list(APPEND YACLIB_COMPILE_OPTIONS --coverage)
  list(APPEND YACLIB_DEFINITIONS NDEBUG)
endif ()
