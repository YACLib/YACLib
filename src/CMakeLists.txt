macro(add_files)
  set(YACLIB_INCLUDES ${YACLIB_INCLUDES} PARENT_SCOPE)
  set(YACLIB_HEADERS ${YACLIB_HEADERS} PARENT_SCOPE)
  set(YACLIB_SOURCES ${YACLIB_SOURCES} PARENT_SCOPE)
endmacro()

configure_file(config.hpp.in ${YACLIB_BINARY_DIR}/include/yaclib/config.hpp)

set(YACLIB_INCLUDE_DIR ${YACLIB_SOURCE_DIR}/include/yaclib)

set(YACLIB_INCLUDES
  ${YACLIB_BINARY_DIR}/include/yaclib/config.hpp
  ${YACLIB_INCLUDE_DIR}/fwd.hpp
  ${YACLIB_INCLUDE_DIR}/log.hpp
  )
set(YACLIB_HEADERS
  )
set(YACLIB_SOURCES
  )

add_subdirectory(algo)
add_subdirectory(async)
add_subdirectory(executor)
add_subdirectory(util)
add_subdirectory(fault)

if (CORO)
  add_subdirectory(coroutine)
endif ()

if (YACLIB_FAULT)
  list(APPEND YACLIB_DEFINITIONS YACLIB_FAULT)
endif ()


if (YACLIB_LOG STREQUAL "INFO")
  list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_info.cpp)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_INFO)
  list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_error.cpp)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_ERROR)
elseif (YACLIB_LOG STREQUAL "ERROR")
  if (YACLIB_TEST)
    list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_info.cpp)
  endif ()
  list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_error.cpp)
  list(APPEND YACLIB_DEFINITIONS YACLIB_LOG_ERROR)
elseif (YACLIB_TEST)
  list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_info.cpp)
  list(APPEND YACLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/log_error.cpp)
endif ()

add_library(${PROJECT_NAME} STATIC)

set(YACLIB_IDE ON)

if (YACLIB_IDE)
  target_sources(${PROJECT_NAME}
    PUBLIC ${YACLIB_INCLUDES}
    PRIVATE ${YACLIB_HEADERS}
    PRIVATE ${YACLIB_SOURCES}
    )
else ()
  target_sources(${PROJECT_NAME}
    PRIVATE ${YACLIB_SOURCES}
    )
endif ()

if (WARN IN_LIST YACLIB_FLAGS)
  include(yaclib_warn_lib)
  message("library warnings: ${YACLIB_WARN}")
endif ()
target_compile_options(${PROJECT_NAME} PRIVATE ${YACLIB_COMPILE_OPTIONS} ${YACLIB_WARN})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${YACLIB_DEFINITIONS})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${YACLIB_SOURCE_DIR}/include
  PUBLIC ${YACLIB_BINARY_DIR}/include # for config.hpp
  PRIVATE ${YACLIB_SOURCE_DIR}/src
  )
