#pragma once

#include "VPUTempReadTypes.h"

namespace vpual {
namespace tempread {
namespace stub {


    RetStatus_t Init();
    RetStatus_t Get(float *css, float *mss, float *upa, float *dss);
    void Deinit();

} // namespace stub
} // namespace tempread
} // namespace vpual
