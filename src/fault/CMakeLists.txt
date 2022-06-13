if (YACLIB_FAULT STREQUAL "FIBER")
  set(YACLIB_FAULT 2 PARENT_SCOPE)
elseif (YACLIB_FAULT STREQUAL "THREAD")
  set(YACLIB_FAULT 1 PARENT_SCOPE)
else ()
  set(YACLIB_FAULT 0)
  set(YACLIB_FAULT 0 PARENT_SCOPE)
endif ()

list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/fault/config.hpp
  ${YACLIB_INCLUDE_DIR}/fault/inject.hpp
  ${YACLIB_INCLUDE_DIR}/fault/injector.hpp
  )

list(APPEND YACLIB_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/util.hpp
  )

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/util.cpp
  )

if (YACLIB_FAULT)
  list(APPEND YACLIB_HEADERS
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic_flag.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/atomic_wait.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/condition_variable.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/condition_variable_any.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/recursive_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/recursive_timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/shared_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/shared_timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/random_device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util.hpp
    )
  list(APPEND YACLIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/random_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/condition_variable.cpp
    )
endif ()

if (YACLIB_FAULT STREQUAL "FIBER")
  list(APPEND YACLIB_HEADERS
    # chrono
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/system_clock.hpp

    # mutexes
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/timed_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/recursive_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/shared_mutex.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/shared_timed_mutex.hpp

    # condition
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/condition_variable.hpp

    # atomic
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/atomic.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/atomic_flag.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/atomic_wait.hpp

    # fiber
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/stack.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/stack_allocator.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/default_allocator.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/fiber_base.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/queue.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/scheduler.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/execution_context.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/thread.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/bidirectional_intrusive_list.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/thread_local_proxy.hpp
    ${YACLIB_INCLUDE_DIR}/fault/detail/fiber/wakeup_helper.hpp
    )
  list(APPEND YACLIB_SOURCES
    # chrono
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/system_clock.cpp

    # thread
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/thread.cpp

    # mutex
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/recursive_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/shared_mutex.cpp

    # condition
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/condition_variable.cpp

    # fibers
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/context/execution_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/context/default_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/fiber_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/bidirectional_intrusive_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/wakeup_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fiber/thread_local_proxy.cpp
    )
endif ()

list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inject.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/injector.cpp
  )

add_files()
