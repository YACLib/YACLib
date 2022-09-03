list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/executor/executor.hpp
  ${YACLIB_INCLUDE_DIR}/executor/inline.hpp
  ${YACLIB_INCLUDE_DIR}/executor/manual.hpp
  ${YACLIB_INCLUDE_DIR}/executor/strand.hpp
  ${YACLIB_INCLUDE_DIR}/executor/submit.hpp
  ${YACLIB_INCLUDE_DIR}/executor/job.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_factory.hpp
  ${YACLIB_INCLUDE_DIR}/executor/thread_pool.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/executor/detail/unique_job.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/fair_thread_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/golang_thread_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/manual.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
  )

add_files()
