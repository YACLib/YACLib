list(APPEND YACLIB_INCLUDES
  ${YACLIB_INCLUDE_DIR}/util/func.hpp
  ${YACLIB_INCLUDE_DIR}/util/helper.hpp
  ${YACLIB_INCLUDE_DIR}/util/intrusive_ptr.hpp
  ${YACLIB_INCLUDE_DIR}/util/ref.hpp
  ${YACLIB_INCLUDE_DIR}/util/result.hpp
  ${YACLIB_INCLUDE_DIR}/util/type_traits.hpp
  )
list(APPEND YACLIB_HEADERS
  ${YACLIB_INCLUDE_DIR}/util/detail/atomic_counter.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/intrusive_ptr_impl.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/node.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/nope_counter.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/safe_call.hpp
  ${YACLIB_INCLUDE_DIR}/util/detail/type_traits_impl.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mpsc_stack.hpp
  )
list(APPEND YACLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mpsc_stack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/node.cpp
  )

add_files()
