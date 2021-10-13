list(APPEND YACLIB_INCLUDES
        ${YACLIB_INCLUDE_DIR}/coroutines/coroutine.h
        ${YACLIB_INCLUDE_DIR}/coroutines/coroutine.h
        ${YACLIB_INCLUDE_DIR}/coroutines/standalone_coroutine.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_allocator.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_view.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/execution_context.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/machine_context.h
        ${YACLIB_INCLUDE_DIR}/coroutines/context/default_allocator.h
        )

list(APPEND YACLIB_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack_builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack.h
        )

SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

list(APPEND YACLIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/coroutine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.cpp
        # todo: pick based on a system
        ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack_x64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x64_mac.S

        ${CMAKE_CURRENT_SOURCE_DIR}/context/default_allocator.cpp
        )

add_files()
