#[[list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/fiber/coroutine.hpp
  )]]

list(APPEND YACLIB_HEADERS
  # chrono
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/steady_clock.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/system_clock.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/other.hpp

  # random
  ${CMAKE_CURRENT_SOURCE_DIR}/random/random_device.hpp

  # thread
  ${CMAKE_CURRENT_SOURCE_DIR}/thread/thread.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread/this_thread.hpp

  # antagonist
  ${CMAKE_CURRENT_SOURCE_DIR}/antagonist/yielder.hpp
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
