/*
 * VPUMemoryWriteBlock.cpp
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

// Includes
//-----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUMemoryWriteBlock
#include <mvLog.h>

#include <RmtUtilsCache.h>
#include <RmtCache_Stub.h>

#include "VPUMemoryWriteBlock.h"

namespace vpual {

// Functions implementation
//-----------------------------------------------------------------------------
MemoryWriteBlock::MemoryWriteBlock(const Utils & utils) :
        utils_{utils}
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(utils.pFrmPoolAlloc != nullptr);
    assert(utils.pRefKeeper != nullptr);

    // Configure pipeline
    std::string inStreamName = "MemoryWriteIn" + std::to_string(Decoder::id);
    std::string outStreamName = "MemoryWriteOut" + std::to_string(Decoder::id);
    std::string releaseStreamName = "MemoryWriteRelease" + std::to_string(Decoder::id);

    PipeMemoryWriteBlock::Configs pipeConfigs;
    pipeConfigs.pInStreamName = inStreamName.c_str();
    pipeConfigs.pOutStreamName = outStreamName.c_str();
    pipeConfigs.outSizeMax = 10 * 1024 * 1024; // FIXME: replace magic number
    pipeConfigs.pReleaseStreamName = releaseStreamName.c_str();

    PipeMemoryWriteBlock::Utils pipeUtils;
    pipeUtils.pFrmPoolAlloc = utils.pFrmPoolAlloc;
    pipeUtils.pRefKeeper = utils.pRefKeeper;

    pipe_.data.Config(&pipeConfigs, &pipeUtils);

    // Start the pipeline
    pipe_.data.Create();
    rmt::utils::cacheFlush(&pipe_, sizeof(pipe_));
    rmt::RemoteCacheInvalidate(&pipe_, sizeof(pipe_));
    pipe_.data.Start();

}
MemoryWriteBlock::~MemoryWriteBlock()
{
    // TODO:
}

void MemoryWriteBlock::Decode(core::Message *cmd, core::Message *rep)
{
    mvLogLevelSet(MVLOG_WARN);
    (void)cmd;
    (void)rep;
}

} // namespace vpual
