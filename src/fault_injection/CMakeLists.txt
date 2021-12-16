list(APPEND YACLIB_INCLUDES
  # chrono
  ${YACLIB_INCLUDE_DIR}/fault_injection/chrono/steady_clock.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/chrono/system_clock.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/chrono/other.hpp

  # random
  ${YACLIB_INCLUDE_DIR}/fault_injection/random/random_device.hpp

  # thread
  ${YACLIB_INCLUDE_DIR}/fault_injection/thread/thread.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/thread/this_thread.hpp

  # mutexes
  ${YACLIB_INCLUDE_DIR}/fault_injection/mutex/mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/mutex/recursive_mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/mutex/shared_mutex.hpp

  # antagonist
  ${YACLIB_INCLUDE_DIR}/fault_injection/antagonist/yielder.hpp
  ${YACLIB_INCLUDE_DIR}/fault_injection/antagonist/inject_fault.hpp
  )

list(APPEND YACLIB_SOURCES
  # chrono
  # TODO(myannyax) create implementation for fibers and pick based on flag
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/steady_clock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/system_clock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/other.cpp

  # random
  ${CMAKE_CURRENT_SOURCE_DIR}/random/random_device.cpp

  # thread
  ${CMAKE_CURRENT_SOURCE_DIR}/thread/thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread/this_thread.cpp

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
  )

add_files()
