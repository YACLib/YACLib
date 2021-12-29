list(APPEND YACLIB_INCLUDES
  # chrono
  ${YACLIB_INCLUDE_DIR}/fault/detail/chrono/steady_clock.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/chrono/system_clock.hpp
  ${YACLIB_INCLUDE_DIR}/fault/chrono.hpp

  # random
  ${YACLIB_INCLUDE_DIR}/fault/detail/random/random_device.hpp
  ${YACLIB_INCLUDE_DIR}/fault/random.hpp

  # thread
  ${YACLIB_INCLUDE_DIR}/fault/detail/thread/thread.hpp
  ${YACLIB_INCLUDE_DIR}/fault/thread.hpp

  # mutexes
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/recursive_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/recursive_timed_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/shared_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/shared_timed_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/mutex/timed_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/shared_mutex.hpp

  # antagonist
  ${YACLIB_INCLUDE_DIR}/fault/detail/antagonist/yielder.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/antagonist/inject_fault.hpp

  # atomics
  ${YACLIB_INCLUDE_DIR}/fault/detail/atomic/atomic.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/atomic/atomic_base.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/atomic/atomic_reference.hpp
  ${YACLIB_INCLUDE_DIR}/fault/detail/atomic/atomic_flag.hpp
  ${YACLIB_INCLUDE_DIR}/fault/atomic.hpp

  # condition
  ${YACLIB_INCLUDE_DIR}/fault/detail/condition/condition_variable.hpp
  ${YACLIB_INCLUDE_DIR}/fault/condition_variable.hpp
  )

list(APPEND YACLIB_SOURCES
  # chrono
  # TODO(myannyax) create implementation for fibers and pick based on flag
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/steady_clock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/system_clock.cpp

  # random
  ${CMAKE_CURRENT_SOURCE_DIR}/random/random_device.cpp

  # thread
  ${CMAKE_CURRENT_SOURCE_DIR}/thread/thread.cpp

  # mutex
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/recursive_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/recursive_timed_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/shared_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/shared_timed_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex/timed_mutex.cpp

  # antagonist
  ${CMAKE_CURRENT_SOURCE_DIR}/antagonist/yielder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/antagonist/inject_fault.cpp

  # condition
  ${CMAKE_CURRENT_SOURCE_DIR}/condition/condition_variable.cpp
  )

add_files()
