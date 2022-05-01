if (YACLIB_FAULT STREQUAL "FIBER")
  set(YACLIB_FAULT 2 PARENT_SCOPE)
elseif (YACLIB_FAULT STREQUAL "THREAD")
  set(YACLIB_FAULT 1 PARENT_SCOPE)
else ()
  set(YACLIB_FAULT 0)
  set(YACLIB_FAULT 0 PARENT_SCOPE)
endif ()

list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/fault/config.hpp
  ${YACLIB_INCLUDE_DIR}/fault/inject.hpp
  ${YACLIB_INCLUDE_DIR}/fault/injector.hpp
  )

if (YACLIB_FAULT)
  list(APPEND YACLIB_HEADERS
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic_flag.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic_wait.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/condition_variable.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/condition_variable_any.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/recursive_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/recursive_timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/shared_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/shared_timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/timed_mutex.hpp
    )
endif ()

if (YACLIB_FAULT STREQUAL "FIBER")
elseif (YACLIB_FAULT STREQUAL "THREAD")
  list(APPEND YACLIB_HEADERS
    ${YACLIB_INCLUDE_DIR}/fault/detail/thread/random_device.hpp
    )
  list(APPEND YACLIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/random_device.cpp
    )
endif ()

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inject.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/injector.cpp
  )

add_files()
