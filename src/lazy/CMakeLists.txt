list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/lazy/make.hpp
  ${YACLIB_INCLUDE_DIR}/lazy/schedule.hpp
  ${YACLIB_INCLUDE_DIR}/lazy/task.hpp
  )
list(APPEND YACLIB_HEADERS
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/task_impl.cpp
  )

add_files()
