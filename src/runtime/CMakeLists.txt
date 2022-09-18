list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/runtime/fair_thread_pool.hpp
  ${YACLIB_INCLUDE_DIR}/runtime/golang_thread_pool.hpp
  )
list(APPEND YACLIB_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/idle.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parker.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/fair_thread_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/golang_thread_pool.cpp
  )

add_files()
