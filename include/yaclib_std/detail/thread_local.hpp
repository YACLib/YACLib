#pragma once

#if YACLIB_FAULT_THREAD_LOCAL == 2
#  include <yaclib/fault/detail/fiber/thread_local_proxy.hpp>

#  define YACLIB_THREAD_LOCAL_PTR(type) yaclib::detail::fiber::ThreadLocalPtrProxy<type>

#else

#  define YACLIB_THREAD_LOCAL_PTR(type) thread_local type*
#endif
