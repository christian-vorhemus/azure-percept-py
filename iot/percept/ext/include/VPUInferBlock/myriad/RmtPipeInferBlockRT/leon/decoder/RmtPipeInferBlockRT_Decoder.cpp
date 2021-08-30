/*
 * RmtPipeInferBlockRT_Decoder.cpp
 *
 *  Created on: Dec 13, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME RmtPipeInferBlockRTDecoder
#include <mvLog.h>

#include "RmtPipeInferBlockRT_Common.h"
#include "RmtPipeInferBlockRT_Decoder.h"

#include <cassert>
#include <cerrno>

#include <LeonRPC_Server.hpp>

#include <PipeInferBlockRT.h>

namespace rmt {

using namespace pipeinferblockrt;
using namespace rmt::utils;

// Global data
// ----------------------------------------------------------------------------
PipeInferBlockRTUtils PipeInferBlockRTDecoder::utils = {};

// Functions implementation
// ----------------------------------------------------------------------------
int32_t PipeInferBlockRTDecoder::decoder(void * pParam)
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
            assert(pCmdMsg->data.cmd.config.pCfg != nullptr);
            assert(pCmdMsg->data.cmd.config.pCfg->data.grpInferCfg.pBlob != nullptr);
            assert(pCmdMsg->data.cmd.config.pCfg->data.grpInferCfg.blobLen != 0);

            // Config pipeline
            cacheInvalidate(pCmdMsg->data.cmd.config.pCfg, sizeof(*pCmdMsg->data.cmd.config.pCfg));
            cacheInvalidate(pCmdMsg->data.cmd.config.pCfg->data.grpInferCfg.pBlob, ROUND_UP(pCmdMsg->data.cmd.config.pCfg->data.grpInferCfg.blobLen, 64));
            pCmdMsg->data.pRmt->Config(&pCmdMsg->data.cmd.config.pCfg->data, &utils);

            break;
        }
    }
    cacheFlush((void *)pCmdMsg, sizeof(*pCmdMsg));

    return ret;
}

std::int32_t PipeInferBlockRTDecoder::Init(PipeInferBlockRTUtils * pUtils)
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
    retRpc = leonrpc::server::Register("PipeInferBlockRT", decoder);
    if (0 != retRpc)
    {
        mvLog(MVLOG_ERROR, "RPC error; retRpc=%d", retRpc);
    }

exit:
    return retRpc;
}

} // namespace rmt
