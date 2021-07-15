macro(set_parent_scope)
  set(${PROJECT_NAME}_INCLUDES ${${PROJECT_NAME}_INCLUDES} PARENT_SCOPE)
  set(${PROJECT_NAME}_HEADERS ${${PROJECT_NAME}_HEADERS} PARENT_SCOPE)
  set(${PROJECT_NAME}_SOURCES ${${PROJECT_NAME}_SOURCES} PARENT_SCOPE)
endmacro()

# yaclib library
add_compile_options(-Wall -Wextra -pedantic)
configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/config.hpp)

set(${PROJECT_NAME}_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/yaclib)

set(${PROJECT_NAME}_INCLUDES
  ${${PROJECT_NAME}_INCLUDE_DIR}/task.hpp
  )
set(${PROJECT_NAME}_HEADERS
  )
set(${PROJECT_NAME}_SOURCES
  )

add_subdirectory(container)
add_subdirectory(executor)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})
target_sources(${PROJECT_NAME}
  PUBLIC ${${PROJECT_NAME}_INCLUDES}
  PRIVATE ${${PROJECT_NAME}_HEADERS}
  )

target_link_libraries(${PROJECT_NAME}
  PUBLIC pthread
)
