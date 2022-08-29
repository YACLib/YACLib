list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/algo/one_shot_event.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_group.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/algo/detail/base_core.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/core.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/inline_core.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/result_core.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/wait_event.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/base_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/one_shot_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/result_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wait_group.cpp
  )

add_files()
