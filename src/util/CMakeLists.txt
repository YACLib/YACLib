list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/util/cast.hpp
  ${YACLIB_INCLUDE_DIR}/util/fail_policy.hpp
  ${YACLIB_INCLUDE_DIR}/util/func.hpp
  ${YACLIB_INCLUDE_DIR}/util/helper.hpp
  ${YACLIB_INCLUDE_DIR}/util/intrusive_ptr.hpp
  ${YACLIB_INCLUDE_DIR}/util/order_policy.hpp
  ${YACLIB_INCLUDE_DIR}/util/ref.hpp
  ${YACLIB_INCLUDE_DIR}/util/result.hpp
  ${YACLIB_INCLUDE_DIR}/util/type_traits.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/util/detail/atomic_counter.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/atomic_event.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/default_deleter.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/default_event.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/intrusive_list.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/intrusive_ptr_impl.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/mutex_event.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/node.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/safe_call.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/set_deleter.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/shared_func.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/type_traits_impl.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/unique_counter.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/result.cpp
  )

if (NOT YACLIB_DISABLE_FUTEX)
  list(APPEND YACLIB_HEADERS
    ${YACLIB_INCLUDE_DIR}/util/detail/atomic_event.hpp
    )
  list(APPEND YACLIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_event.cpp
    )
endif ()

add_files()
