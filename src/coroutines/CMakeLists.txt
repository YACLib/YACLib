list(APPEND YACLIB_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/coroutine.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack_allocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack_view.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.h
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack_builder.h
        )
list(APPEND YACLIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/coroutine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.cpp
        )

enable_language(ASM)

set(ASM_SOURCES
        context/asm/switch_context.S
        )

add_files()
