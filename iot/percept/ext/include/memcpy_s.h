/*
* Copyright 2017 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
*/

#ifndef _MEMCPY_S_H_
#define _MEMCPY_S_H_

#ifdef __PC__
#include <errno.h>
// try to use system defined memcpy_s if available
#if defined(__STDC_LIB_EXT1__) && (__STDC_LIB_EXT1__ >= 201112L)
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#else
#ifndef RSIZE_MAX
#define RSIZE_MAX (SIZE_MAX >> 1)
#endif

inline static int memcpy_s(void * dest, size_t destsz, const void * const src, size_t count) {
    if (dest == NULL) return EINVAL; // dest should not be a NULL ptr
    if (destsz > RSIZE_MAX) return ERANGE;
    if (count > RSIZE_MAX) return ERANGE;
    if (destsz < count) { memset(dest, 0, destsz); return ERANGE; }
    if (src == NULL) { memset(dest, 0, destsz); return EINVAL; } // src should not be a NULL ptr

    // Copying shall not take place between regions that overlap.
    if( ((dest > src) && ((char*)dest < ((char*)src + count))) ||
            ((src > dest) && ((char*)src < ((char*)dest + destsz))) ) {
        memset(dest, 0, destsz);
        return ERANGE;
    }

    memcpy(dest, src, count);
    return 0;
}
#endif // __STDC_LIB_EXT1__

#endif // __PC__

#endif // _MEMCPY_S_H_

