/*
 * RmtPipeInferBlockRT_Stub.cpp
 *
 *  Created on: Dec 13, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME RmtPipeInferBlockRTStub
#include <mvLog.h>

#include "RmtPipeInferBlockRT_Common.h"
#include "RmtPipeInferBlockRT_Stub.h"

#include <cassert>

#include <LeonRPC_Client.hpp>
#include <RmtUtilsCache.h>

namespace rmt {

using namespace utils;
using namespace pipeinferblockrt;

// Functions implementation
// ----------------------------------------------------------------------------
static std::int32_t rmtDecoder(CacheAligned<CmdMsg> * pCmdMsg)
{
    leonrpc::client::Procedure rpc("PipeInferBlockRT");
    return rpc((void *)pCmdMsg);
}

PipeInferBlockRTStub::PipeInferBlockRTStub():
        pRmt(nullptr)
{};

void PipeInferBlockRTStub::Bind(PipeInferBlockRT * pRmt)
{
    assert(pRmt != nullptr);
    // Bind both child and parent interface
    this->pRmt = pRmt;
    IFlicPipeWrapStub::Bind(pRmt);
};


void PipeInferBlockRTStub::Config(rmt::utils::CacheAligned<PipeInferBlockRTCfg> * pCfg)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pCfg != nullptr);

    // Prepare command message
    CacheAligned<CmdMsg> cmdMsg;
    cmdMsg.data.cmdId = CmdId::Config;
    cmdMsg.data.pRmt = pRmt;
    cacheFlush(pCfg, sizeof(*pCfg));
    cmdMsg.data.cmd.config.pCfg = pCfg;
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
