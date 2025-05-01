macro(add_files)
  set(YACLIB_INCLUDES ${YACLIB_INCLUDES} PARENT_SCOPE)
  set(YACLIB_HEADERS ${YACLIB_HEADERS} PARENT_SCOPE)
  set(YACLIB_SOURCES ${YACLIB_SOURCES} PARENT_SCOPE)
endmacro()

set(YACLIB_INCLUDE_DIR ${YACLIB_SOURCE_DIR}/include/yaclib)
set(YACLIB_STD_INCLUDE_DIR ${YACLIB_SOURCE_DIR}/include/yaclib_std)

set(YACLIB_INCLUDES
  ${YACLIB_BINARY_DIR}/include/yaclib/config.hpp
  ${YACLIB_INCLUDE_DIR}/fwd.hpp
  ${YACLIB_INCLUDE_DIR}/log.hpp
  )
set(YACLIB_HEADERS
  )
set(YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  )

add_subdirectory(algo)
add_subdirectory(async)
add_subdirectory(exe)
add_subdirectory(fault)
add_subdirectory(lazy)
add_subdirectory(runtime)
add_subdirectory(std)
add_subdirectory(util)

if (YACLIB_CORO_NEED)
  add_subdirectory(coro)
endif ()

configure_file(config.hpp.in ${YACLIB_BINARY_DIR}/include/yaclib/config.hpp)

add_library(yaclib STATIC)

target_sources(yaclib
  PUBLIC ${YACLIB_INCLUDES}
  PRIVATE ${YACLIB_HEADERS}
  PRIVATE ${YACLIB_SOURCES}
  )

if (WARN IN_LIST YACLIB_FLAGS)
  include(yaclib_warn_lib)
  message("YACLib/library warnings: ${YACLIB_WARN}")
endif ()
target_compile_options(yaclib PRIVATE ${YACLIB_COMPILE_OPTIONS} ${YACLIB_WARN})
target_compile_definitions(yaclib PUBLIC ${YACLIB_DEFINITIONS})

find_package(Threads REQUIRED)
target_link_libraries(yaclib PUBLIC Threads::Threads)

target_include_directories(yaclib
  PUBLIC ${YACLIB_SOURCE_DIR}/include
  PUBLIC ${YACLIB_BINARY_DIR}/include # for config.hpp
  PRIVATE ${YACLIB_SOURCE_DIR}/src
  )

if (YACLIB_INSTALL)
  install(TARGETS yaclib)
endif ()
