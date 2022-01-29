list(APPEND YACLIB_INCLUDES
  # chrono
  ${YACLIB_INCLUDE_DIR}/fault/chrono.hpp

  # random
  ${YACLIB_INCLUDE_DIR}/fault/random.hpp

  # thread
  ${YACLIB_INCLUDE_DIR}/fault/thread.hpp

  # mutexes
  ${YACLIB_INCLUDE_DIR}/fault/mutex.hpp
  ${YACLIB_INCLUDE_DIR}/fault/shared_mutex.hpp

  # atomics
  ${YACLIB_INCLUDE_DIR}/fault/atomic.hpp

  # condition
  ${YACLIB_INCLUDE_DIR}/fault/condition_variable.hpp
  )

add_files()
