list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/algo/one_shot_event.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_for.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_group.hpp
  ${YACLIB_INCLUDE_DIR}/algo/wait_until.hpp
  ${YACLIB_INCLUDE_DIR}/algo/when_all.hpp
  ${YACLIB_INCLUDE_DIR}/algo/when_any.hpp
  ${YACLIB_INCLUDE_DIR}/algo/when_policy.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/algo/detail/wait_impl.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/when_all_impl.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/when_any_impl.hpp
  ${YACLIB_INCLUDE_DIR}/algo/detail/when_impl.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/one_shot_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wait_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wait_impl.cpp
  )

add_files()
