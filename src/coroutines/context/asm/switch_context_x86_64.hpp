#pragma once

#if defined(__APPLE__)
#define YACLIB_FUNC_NAME(x) _##x
#else
#define YACLIB_FUNC_NAME(x) x
#endif

#define YACLIB_RBX_INDEX 0
#define YACLIB_RBP_INDEX 1
#define YACLIB_R12_INDEX 2
#define YACLIB_R13_INDEX 3
#define YACLIB_R14_INDEX 4
#define YACLIB_R15_INDEX 5
#define YACLIB_RDI_INDEX 6
#define YACLIB_RSI_INDEX 7
#define YACLIB_RSP_INDEX 8
#define YACLIB_RIP_INDEX 9
