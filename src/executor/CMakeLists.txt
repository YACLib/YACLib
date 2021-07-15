list(APPEND ${PROJECT_NAME}_INCLUDES
  ${${PROJECT_NAME}_INCLUDE_DIR}/executor/executor.hpp
  ${${PROJECT_NAME}_INCLUDE_DIR}/executor/thread_pool.hpp
  ${${PROJECT_NAME}_INCLUDE_DIR}/executor/thread_factory.hpp
  )
list(APPEND ${PROJECT_NAME}_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thread_factory.cpp
  )

set_parent_scope()
