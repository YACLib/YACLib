list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/coroutines/standalone_coroutine.hpp
  ${YACLIB_INCLUDE_DIR}/coroutines/context/stack.hpp
  ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_allocator.hpp
  ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_view.hpp
  ${YACLIB_INCLUDE_DIR}/coroutines/context/default_allocator.hpp
  )

list(APPEND YACLIB_HEADERS
  # todo: pick based on a system
  ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x86_64.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack_x64.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/coroutine_impl.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine_impl.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine_factory.hpp
  )

SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/coroutine_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine_impl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/context/default_allocator.cpp
  # todo: pick based on a system
  ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack_x64.cpp
  )

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x86_64.S
  )

add_files()
