/*
 * VPUTelemetry.cpp
 *
 *  Created on: Jan 15, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUTelemetry
#include <mvLog.h>

#include "VPUTelemetry.h"
#include "VPUTelemetryVpualCommon.h"

#include <cassert>
#include <VpualDispatcher.h>

namespace vpual {
namespace telemetry {

// Local data
// ----------------------------------------------------------------------------
namespace {
static vpual::core::Stub *pVpualStub = nullptr;
};

// Functions implementation
// ----------------------------------------------------------------------------
int Init()
{
    mvLogLevelSet(MVLOG_WARN);

    assert(pVpualStub == nullptr);

    mvLog(MVLOG_INFO, "Initializing Telemetry");
    pVpualStub = new core::Stub("Telemetry");
    if (pVpualStub == nullptr)
    {
        mvLog(MVLOG_ERROR, "Could not construct for VPUAL stub");
        return ENOMEM;
    }

    Command command = Command::INIT;
    core::Message request, response;
    request.serialize(&command, sizeof(command));
    pVpualStub->dispatch(request, response);

    std::int32_t decoderRet;
    response.deserialize(&decoderRet, sizeof(decoderRet));

    if (0 != decoderRet)
    {
        mvLog(MVLOG_ERROR, "Decoder error: unknown command");
        return -1;
    }

    std::int32_t commandRet;
    response.deserialize(&commandRet, sizeof(commandRet));
    if (0 != commandRet)
    {
        mvLog(MVLOG_ERROR, "Command error: %d", commandRet);
        return -1;
    }

    return 0;
}

int Deinit()
{
    mvLogLevelSet(MVLOG_WARN);

    assert(pVpualStub != nullptr);

    mvLog(MVLOG_INFO, "Deinitializing Telemetry");
    Command command = Command::DEINIT;
    core::Message request, response;
    request.serialize(&command, sizeof(command));
    pVpualStub->dispatch(request, response);

    std::uint32_t decoderRet;
    response.deserialize(&decoderRet, sizeof(decoderRet));

    if (0 != decoderRet)
    {
        mvLog(MVLOG_ERROR, "Decoder error: unknown command");
        return -1;
    }

    std::int32_t commandRet;
    response.deserialize(&commandRet, sizeof(commandRet));
    if (0 != commandRet)
    {
        mvLog(MVLOG_ERROR, "Command error: %d", commandRet);
        return -1;
    }

    delete pVpualStub;
    pVpualStub = nullptr;

    return 0;
}

std::int32_t Memory::Get(Memory::Info * pEntry)
{
    mvLogLevelSet(MVLOG_WARN);
    assert(pVpualStub != nullptr);

    // Send command to decoder
    Command command = Command::GETMEMORY;
    core::Message request, response;
    request.serialize(&command, sizeof(command));
    pVpualStub->dispatch(request, response);

    // Get decoder response
    std::int32_t decoderRet;
    response.deserialize(&decoderRet, sizeof(decoderRet));
    if (0 != decoderRet)
    {
        mvLog(MVLOG_ERROR, "Decoder error: %d", decoderRet);
        return -1; //FIXME: define return type
    }

    // Get the command status
    std::int32_t commandRet;
    response.deserialize(&commandRet, sizeof(commandRet));
    if (0 != commandRet)
    {
        mvLog(MVLOG_ERROR, "Command error: %d", commandRet);
        return -1; //FIXME: define return type
    }

    // Get the response data
    std::uint32_t val;

    response.deserialize(&val, sizeof(val));
    pEntry->poolMain.used = val;
    response.deserialize(&val, sizeof(val));
    pEntry->poolMain.total = val;

    response.deserialize(&val, sizeof(val));
    pEntry->losHeap.used = val;
    response.deserialize(&val, sizeof(val));
    pEntry->losHeap.total = val;

    response.deserialize(&val, sizeof(val));
    pEntry->lrtHeap.used = val;
    response.deserialize(&val, sizeof(val));
    pEntry->lrtHeap.total = val;

    return 0;
}

void Memory::Print(const Info & info)
{
    printf("MX memory telemetry:\n");
    printf("Pool main:   %10u/%10u (used/total)\n", info.poolMain.used, info.poolMain.total);
    printf("LeonOS heap: %10u/%10u (used/total)\n", info.losHeap.used, info.losHeap.total);
    printf("LeonRT heap: %10u/%10u (used/total)\n", info.lrtHeap.used, info.lrtHeap.total);

    printf("\n");
}

} // namespace telemetry
} // namespace vpual
