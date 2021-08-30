/*
 * RmtPipeCameraBlockRT_Stub.cpp
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME RmtPipeCameraBlockRTStub
#include <mvLog.h>

#include "RmtPipeCameraBlockRT_Common.h"
#include "RmtPipeCameraBlockRT_Stub.h"

#include <cassert>

#include <LeonRPC_Client.hpp>
#include <RmtUtilsCache.h>

namespace rmt {

using namespace utils;
using namespace pipecamerablockrt;

// Functions implementation
// ----------------------------------------------------------------------------
static std::int32_t rmtDecoder(CacheAligned<CmdMsg> * pCmdMsg)
{
    leonrpc::client::Procedure rpc("PipeCameraBlockRT");
    return rpc((void *)pCmdMsg);
}

PipeCameraBlockRTStub::PipeCameraBlockRTStub():
        pRmt(nullptr)
{};

void PipeCameraBlockRTStub::Bind(PipeCameraBlockRT * pRmt)
{
    assert(pRmt != nullptr);
    // Bind both child and parent interface
    this->pRmt = pRmt;
    IFlicPipeWrapStub::Bind(pRmt);
};


void PipeCameraBlockRTStub::Config(rmt::utils::CacheAligned<PipeCameraBlockRT::Configs> * pConfigs)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pConfigs != nullptr);
    assert(pConfigs->data.grpVidEncCfg.pVidEncSettings != nullptr);

    // Prepare command message
    CacheAligned<CmdMsg> cmdMsg;
    cmdMsg.data.cmdId = CmdId::Config;
    cmdMsg.data.pRmt = pRmt;
    cacheFlush(pConfigs, sizeof(*pConfigs));
    cacheFlush(pConfigs->data.grpVidEncCfg.pVidEncSettings, sizeof(*pConfigs->data.grpVidEncCfg.pVidEncSettings));
    cmdMsg.data.cmd.config.pConfigs = pConfigs;
    cacheFlush((void *)&cmdMsg, sizeof(cmdMsg));

    // Remote decoder call
    std::int32_t retRpc = 0;
    retRpc = rmtDecoder(&cmdMsg);
    if (0 != retRpc)
    {
        mvLog(MVLOG_ERROR, "RPC error; retRpc = %d", retRpc);
        goto exit;
    }

exit:
    return;
};

} // namespace rmt
