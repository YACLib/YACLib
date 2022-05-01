#pragma once

#if YACLIB_FAULT_COUNTING_SEMAPHORE == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"
#elif YACLIB_FAULT_COUNTING_SEMAPHORE == 1  // TODO(myannyax) Implement, needs ifdef because these from C++20
#  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <atomic>
#endif
