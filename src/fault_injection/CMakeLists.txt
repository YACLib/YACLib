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

  # antagonist
  ${YACLIB_INCLUDE_DIR}/fault_injection/antagonist/yielder.hpp
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

  # antagonist
  ${CMAKE_CURRENT_SOURCE_DIR}/antagonist/yielder.cpp
  )

add_files()
