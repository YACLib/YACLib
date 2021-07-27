list(
  APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/container/intrusive_node.hpp
  ${YACLIB_INCLUDE_DIR}/container/intrusive_ptr.hpp
  ${YACLIB_INCLUDE_DIR}/container/counter.hpp
)
list(
  APPEND YACLIB_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.hpp
)
list(
  APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mpsc_stack.hpp
)

add_files()
