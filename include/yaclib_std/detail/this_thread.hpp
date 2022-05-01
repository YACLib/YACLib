#pragma once

#if YACLIB_FAULT_THIS_THREAD == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"
#elif YACLIB_FAULT_THIS_THREAD == 1  // TODO(myannyax) Maybe implement
#  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <thread>

namespace yaclib_std::this_thread {

using std::this_thread::sleep_for;

using std::this_thread::sleep_until;

using std::this_thread::yield;

using std::this_thread::get_id;

}  // namespace yaclib_std::this_thread
#endif
