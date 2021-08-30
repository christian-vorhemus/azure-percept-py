/*
 * RmtPipeCameraBlockRT_Decoder.cpp
 *
 *  Created on: Feb 14, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME RmtPipeCameraBlockRTDecoder
#include <mvLog.h>

#include "RmtPipeCameraBlockRT_Common.h"
#include "RmtPipeCameraBlockRT_Decoder.h"

#include <cassert>
#include <cerrno>
#include <LeonRPC_Server.hpp>
#include <PipeCameraBlockRT.h>

namespace rmt {

using namespace pipecamerablockrt;
using namespace rmt::utils;

// Global data
// ----------------------------------------------------------------------------
PipeCameraBlockRT::Utils PipeCameraBlockRTDecoder::utils = {};

// Functions implementation
// ----------------------------------------------------------------------------
int32_t PipeCameraBlockRTDecoder::decoder(void * pParam)
{
    mvLogLevelSet(MVLOG_WARN);
    int32_t ret = 0;

    // Sanity checks
    assert(nullptr != pParam);

    CacheAligned<CmdMsg> * pCmdMsg = (CacheAligned<CmdMsg> *)pParam;
    cacheInvalidate((void *)pCmdMsg, sizeof(*pCmdMsg));

    mvLog(MVLOG_DEBUG, "cmdId = %d", pCmdMsg->data.cmdId);
    switch(pCmdMsg->data.cmdId)
    {
        default:
        {
            ret = ENOSYS;
            mvLog(MVLOG_ERROR, "cmdId %u not implemented", pCmdMsg->data.cmdId);
            break;
        }

        case CmdId::Config:
        {
            // Sanity checks
            assert(pCmdMsg->data.pRmt != nullptr);
            assert(pCmdMsg->data.cmd.config.pConfigs != nullptr);
            cacheInvalidate(pCmdMsg->data.cmd.config.pConfigs,
                            sizeof(*pCmdMsg->data.cmd.config.pConfigs));
            assert(pCmdMsg->data.cmd.config.pConfigs->data.grpVidEncCfg.pVidEncSettings != nullptr);
            cacheInvalidate(pCmdMsg->data.cmd.config.pConfigs->data.grpVidEncCfg.pVidEncSettings,
                            sizeof(*pCmdMsg->data.cmd.config.pConfigs->data.grpVidEncCfg.pVidEncSettings));

            // Config pipeline
            pCmdMsg->data.pRmt->Config(&pCmdMsg->data.cmd.config.pConfigs->data, &utils);

            break;
        }
    }
    cacheFlush((void *)pCmdMsg, sizeof(*pCmdMsg));

    return ret;
}

std::int32_t PipeCameraBlockRTDecoder::Init(PipeCameraBlockRT::Utils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pUtils != nullptr);

    // Copy utils
    utils = *pUtils;

    int32_t retRpc = 0;

    // Register parent interface RPC decoder
    retRpc = IFlicPipeWrapDecoder::RegisterDecoder();
    if (0 != retRpc)
        goto exit;

    // Register RPC decoder
    retRpc = leonrpc::server::Register("PipeCameraBlockRT", decoder);
    if (0 != retRpc)
    {
        mvLog(MVLOG_ERROR, "RPC error; retRpc=%d", retRpc);
    }

exit:
    return retRpc;
}

} // namespace rmt
