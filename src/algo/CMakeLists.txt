list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/algo/wait.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_for.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_until.hpp
  ${YACLIB_INCLUDE_DIR}/algo/when_all.hpp
  ${YACLIB_INCLUDE_DIR}/algo/when_any.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/algo/detail/wait.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/wait.cpp
  )

add_files()
