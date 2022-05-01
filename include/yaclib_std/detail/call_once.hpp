#pragma once

#if YACLIB_FAULT_CALL_ONCE == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"
#elif YACLIB_FAULT_CALL_ONCE == 1  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <mutex>
#endif
