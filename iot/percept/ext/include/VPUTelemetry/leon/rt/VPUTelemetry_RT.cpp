/*
 * VPUTelemetry_RT.cpp
 *
 *  Created on: Jan 20, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUTelemetry
#include <mvLog.h>

#include <rtems/malloc.h>
#include <LeonRPC_Server.hpp>
#include <RmtUtilsCache.h>

namespace vpual {
namespace telemetry {

// Functions implementation
// ----------------------------------------------------------------------------
static int32_t malloc_info_rpc(void *param)
{
    mvLogLevelSet(MVLOG_WARN);
    rmt::utils::CacheAligned<Heap_Information_block> * heapInfo =
            (rmt::utils::CacheAligned<Heap_Information_block> *)param;
    // malloc_info is provided by RTEMS
    malloc_info(&heapInfo->data);
    rmt::utils::cacheFlush(heapInfo, sizeof(*heapInfo));

    return 0;
}

int Init()
{
    leonrpc::server::Register("malloc_info", malloc_info_rpc);
    return 0;
}

} // namespace telemetry
} // namespace vpual
