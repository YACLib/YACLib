list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/executor/executor.hpp
  ${YACLIB_INCLUDE_DIR}/executor/inline.hpp
  ${YACLIB_INCLUDE_DIR}/executor/strand.hpp
  ${YACLIB_INCLUDE_DIR}/executor/task.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_factory.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_pool.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/inline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
  )

add_files()
