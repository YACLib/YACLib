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
  )

list(APPEND YACLIB_SOURCES
  # chrono
  # TODO(myannyax) create implementation for fibers and pick based on flag
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/steady_clock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/system_clock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono/other.cpp

  # random
  ${CMAKE_CURRENT_SOURCE_DIR}/random/random_device.cpp
  )

add_files()
