#ifndef ___STDINT_H__
#define ___STDINT_H__

#if defined(__WATCOMC__) && (__WATCOMC__ < 1200)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
#else
#include <stdint.h>
#endif // defined(__WATCOMC__) && (__WATCOMC__ < 1200)

#endif // ___STDINT_H__
