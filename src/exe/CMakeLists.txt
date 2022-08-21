list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/exe/executor.hpp
  ${YACLIB_INCLUDE_DIR}/exe/inline.hpp
  ${YACLIB_INCLUDE_DIR}/exe/job.hpp
  ${YACLIB_INCLUDE_DIR}/exe/manual.hpp
  ${YACLIB_INCLUDE_DIR}/exe/strand.hpp
  ${YACLIB_INCLUDE_DIR}/exe/submit.hpp
  ${YACLIB_INCLUDE_DIR}/exe/thread_factory.hpp
  ${YACLIB_INCLUDE_DIR}/exe/thread_pool.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/exe/detail/unique_job.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/inline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/manual.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
  )

add_files()
