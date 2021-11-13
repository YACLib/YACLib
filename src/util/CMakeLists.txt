list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/util/counters.hpp
  ${YACLIB_INCLUDE_DIR}/util/func.hpp
  ${YACLIB_INCLUDE_DIR}/util/intrusive_node.hpp
  ${YACLIB_INCLUDE_DIR}/util/intrusive_ptr.hpp
  ${YACLIB_INCLUDE_DIR}/util/ref.hpp
  ${YACLIB_INCLUDE_DIR}/util/result.hpp
  ${YACLIB_INCLUDE_DIR}/util/type_traits.hpp
  )
list(APPEND YACLIB_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mpsc_stack.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mpsc_stack.cpp
  )

add_files()
