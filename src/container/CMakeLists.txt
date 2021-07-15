list(APPEND ${PROJECT_NAME}_INCLUDES
  ${${PROJECT_NAME}_INCLUDE_DIR}/container/intrusive_node.hpp
  )
list(APPEND ${PROJECT_NAME}_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.hpp
  )
list(APPEND ${PROJECT_NAME}_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.cpp
  )

set_parent_scope()
