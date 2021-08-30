/*
 * VPUTelemetry_OS.cpp
 *
 *  Created on: Jan 19, 2021
 *      Author: apalfi
 */

// Includes
// -------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUTelemetry
#include <mvLog.h>

#include "VPUTelemetry_OS.h"
#include "VPUTelemetryVpualCommon.h"

#include <rtems/malloc.h>
#include <MemAllocator.h>
#include <RmtUtilsCache.h>
#include <LeonRPC_Client.hpp>

// External data
// -------------------------------------------------------------------------------------
extern RgnAllocator gAppMainAlloc; //FIXME: provide this in constructor

namespace vpual {
namespace telemetry {

// Functions implementation
// -------------------------------------------------------------------------------------
void VPUTelemetry::Decode(core::Message *request, core::Message *response)
{
    mvLogLevelSet(MVLOG_WARN);
    Command command;
    request->deserialize(&command, sizeof(command));

    std::int32_t decoderRet;

    switch(command) {
        case Command::INIT:
            decoderRet = 0;
            response->serialize(&decoderRet, sizeof(decoderRet));
            init(response);
            break;
        case Command::DEINIT:
            decoderRet = 0;
            response->serialize(&decoderRet, sizeof(decoderRet));
            deinit(response);
            break;
        case Command::GETMEMORY:
            decoderRet = 0;
            response->serialize(&decoderRet, sizeof(decoderRet));
            getMemory(response);
            break;

        default:
            decoderRet = -1; //FIXME: add enum value
            response->serialize(&decoderRet, sizeof(decoderRet));
            mvLog(MVLOG_ERROR,"Unknown command %d received",command);
            break;
    }
}

void VPUTelemetry::init(core::Message *response)
{
    mvLogLevelSet(MVLOG_WARN);
    std::int32_t commandStatus;
    commandStatus = 0;
    response->serialize(&commandStatus, sizeof(commandStatus));

    // Nothing to do yet
}

void VPUTelemetry::deinit(core::Message *response)
{
    mvLogLevelSet(MVLOG_WARN);
    std::int32_t commandStatus;
    commandStatus = 0;
    response->serialize(&commandStatus, sizeof(commandStatus));

    // Nothing to do yet
}

void VPUTelemetry::getMemory(core::Message *response)
{
    mvLogLevelSet(MVLOG_WARN);
    std::int32_t commandStatus;
    commandStatus = 0;
    response->serialize(&commandStatus, sizeof(commandStatus));

    // Main pool
    std::uint32_t val = 0;
    val = gAppMainAlloc.getUsedSz();
    response->serialize(&val, sizeof(val));
    val = gAppMainAlloc.getRgnSz();
    response->serialize(&val, sizeof(val));

    // LeonOS HEAP
    // malloc_info is provided by RTEMS
    Heap_Information_block heapInfo;
    malloc_info(&heapInfo);

    val = heapInfo.Used.total;
    response->serialize(&val, sizeof(val));
    val = heapInfo.Used.total + heapInfo.Free.total;
    response->serialize(&val, sizeof(val));

    // LeonRT HEAP
    rmt::utils::CacheAligned<Heap_Information_block>  heapInfoLrt;
    leonrpc::client::Procedure malloc_info_rpc("malloc_info");
    malloc_info_rpc(&heapInfoLrt);
    rmt::utils::cacheInvalidate(&heapInfoLrt, sizeof(heapInfoLrt));
    val = heapInfoLrt.data.Used.total;
    response->serialize(&val, sizeof(val));
    val = heapInfoLrt.data.Used.total + heapInfoLrt.data.Free.total;
    response->serialize(&val, sizeof(val));
}

} // namespace telemetry
} // namespace vpual
