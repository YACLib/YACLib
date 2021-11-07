list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/fiber/coroutine.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/coroutine_base.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/stack.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/stack_allocator.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/stack_view.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/default_allocator.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/detail/execution_context.hpp
  ${YACLIB_INCLUDE_DIR}/fiber/detail/setup_stack_x64.hpp
  )

list(APPEND YACLIB_HEADERS
  # TODO(myannyax): pick based on a system
  ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x86_64.hpp
  )

SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/coroutine_base.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/coroutine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/default_allocator.cpp
  # TODO(myannyax): pick based on a system
  ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack_x64.cpp
  )

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x86_64.S
  )

add_files()
