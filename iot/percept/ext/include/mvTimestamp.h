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

#ifndef _MV_TIMESTAMP_H_
#define _MV_TIMESTAMP_H_

#include <time.h>
#ifdef __cplusplus
extern "C"{
#endif
int MvTimespecDiff(struct timespec* diff,
                   const struct timespec *a,
                   const struct timespec *b);
int MvTimespecDiffNs(int64_t *diff,
                     const struct timespec *a,
                     const struct timespec *b);
int MvTimespecDiffNsAbs(int64_t *diff,
                        const struct timespec *a,
                        const struct timespec *b);

int MvNanosToTimespec(struct timespec *ts, const int64_t ns);
int MvTimespecToNanos(int64_t *ns,
                      const struct timespec* currentTime);

int MvGetTimestamp(struct timespec* currentTime);
#ifdef __cplusplus
}
#endif
#endif // _MV_TIMESTAMP_H_
