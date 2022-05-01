#pragma once

#if YACLIB_FAULT_ATOMIC_OP == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"
#elif YACLIB_FAULT_ATOMIC_OP == 1  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <atomic>
#endif
