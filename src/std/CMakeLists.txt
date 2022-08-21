list(APPEND YACLIB_INCLUDES
  ${YACLIB_STD_INCLUDE_DIR}/atomic
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic_fence.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic_flag.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic_flag_op.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic_op.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/atomic_ref.hpp

  ${YACLIB_STD_INCLUDE_DIR}/barrier
  ${YACLIB_STD_INCLUDE_DIR}/detail/barrier.hpp

  ${YACLIB_STD_INCLUDE_DIR}/chrono
  ${YACLIB_STD_INCLUDE_DIR}/detail/clock.hpp

  ${YACLIB_STD_INCLUDE_DIR}/condition_variable
  ${YACLIB_STD_INCLUDE_DIR}/detail/condition_variable.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/condition_variable_any.hpp

  ${YACLIB_STD_INCLUDE_DIR}/latch
  ${YACLIB_STD_INCLUDE_DIR}/detail/latch.hpp

  ${YACLIB_STD_INCLUDE_DIR}/mutex
  ${YACLIB_STD_INCLUDE_DIR}/detail/call_once.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/mutex.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/recursive_mutex.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/recursive_timed_mutex.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/timed_mutex.hpp

  ${YACLIB_STD_INCLUDE_DIR}/semaphore
  ${YACLIB_STD_INCLUDE_DIR}/detail/binary_semaphore.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/counting_semaphore.hpp

  ${YACLIB_STD_INCLUDE_DIR}/shared_mutex
  ${YACLIB_STD_INCLUDE_DIR}/detail/shared_mutex.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/shared_timed_mutex.hpp

  ${YACLIB_STD_INCLUDE_DIR}/thread
  ${YACLIB_STD_INCLUDE_DIR}/detail/jthread.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/this_thread.hpp
  ${YACLIB_STD_INCLUDE_DIR}/detail/thread.hpp

  ${YACLIB_STD_INCLUDE_DIR}/thread_local
  )

add_files()
