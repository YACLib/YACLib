list(APPEND YACLIB_INCLUDES
        ${YACLIB_INCLUDE_DIR}/coroutines/coroutine.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/coroutine.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/standalone_coroutine.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_allocator.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/stack_view.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/execution_context.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/machine_context.hpp
        ${YACLIB_INCLUDE_DIR}/coroutines/context/default_allocator.hpp
        )

list(APPEND YACLIB_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/context/stack_builder.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack.hpp
        )

SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

list(APPEND YACLIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/coroutine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/standalone_coroutine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/execution_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/context/machine_context.cpp
        # todo: pick based on a system
        ${CMAKE_CURRENT_SOURCE_DIR}/context/setup_stack/setup_stack_x64.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/context/default_allocator.cpp
        )

if(WIN32)
    # do nothing
elseif(APPLE)
    list(APPEND YACLIB_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x64_mac.S
            )
else()
    list(APPEND YACLIB_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/context/asm/switch_context_x64_linux.S
            )
endif()

add_files()
