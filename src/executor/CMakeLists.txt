list(
  APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/executor/serial.hpp
  ${YACLIB_INCLUDE_DIR}/executor/executor.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_factory.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_pool.hpp
  ${YACLIB_INCLUDE_DIR}/executor/inline.hpp
)
list(
  APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/serial.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inline.cpp
)

add_files()
