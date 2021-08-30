///
/// @file      secure_functions.h
///
/// @brief     Secure version of memcpy.
///

#ifndef __SECURE_FUNCTIONS_H__
#define __SECURE_FUNCTIONS_H__

#ifndef MA2X8X

#include <errno.h> // for errno
#include <stdint.h> // for size_t
#include <string.h> // for memset, memcpy

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

inline static int memcpy_s (void * dest, size_t destsz, const void * const src, size_t count)
{
    if (dest == NULL) return EINVAL; // dest should not be a NULL ptr
    if (destsz > SIZE_MAX) return ERANGE;
    if (count > SIZE_MAX) return ERANGE;
    if (destsz < count) { memset(dest, 0, destsz); return ERANGE; }
    if (src == NULL) { memset(dest, 0, destsz); return EINVAL; } // src should not be a NULL ptr

    memcpy(dest, src, count);
    return 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MA2X8X

#endif /* __SECURE_FUNCTIONS_H__ */
