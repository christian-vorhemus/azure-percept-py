/*
 * VPUMemoryWriteBlock.cpp
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUMemoryWriteBlock
#include <mvLog.h>

#include "VPUMemoryWriteBlock.h"

#include <cassert>
#include <VPUBlockXLink.h>

// Functions implementation
// ----------------------------------------------------------------------------
namespace vpual {

MemoryWriteBlock::MemoryWriteBlock():
        Stub("MemoryWriteBlock")
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    if (Stub::id_ == 0)
    {
        mvLog(MVLOG_FATAL, "Could not create MemoryWriteBlock stub\n");
        exit(1);
    }

    auto xLinkHandler = getXlinkDeviceHandler(0);
    std::string streamName;

    // Open XLink streams
    streamName = "MemoryWriteIn" + std::to_string(Stub::id_);
    inStream_.Open(xLinkHandler.linkId, streamName.c_str(), 20*1024*1024); // FIXME: provide as parameter
    streamName = "MemoryWriteOut" + std::to_string(Stub::id_);
    outStream_.Open(xLinkHandler.linkId, streamName.c_str());
    streamName = "MemoryWriteRelease" + std::to_string(Stub::id_);
    releaseStream_.Open(xLinkHandler.linkId, streamName.c_str());
}

MemoryWriteBlock::~MemoryWriteBlock()
{
    // TODO: add implementation
}

RmtMemHndl MemoryWriteBlock::Write(const Frame & frame)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(frame.buffer.base != nullptr);
    assert(frame.buffer.size != 0);
    assert(frame.rmtMemHndl == 0);

    // Send payload
    inStream_.Write(frame);

    // Receive memory handle
    Frame outFrame;
    outStream_.Read(&outFrame);
    mvLog(MVLOG_DEBUG,"rmtMemHndl = 0x%x", outFrame.rmtMemHndl);

    return outFrame.rmtMemHndl;
}

void MemoryWriteBlock::Release(const RmtMemHndl rmtMemHndl)
{
    mvLogLevelSet(MVLOG_WARN);
    mvLog(MVLOG_DEBUG,"rmtMemHndl = 0x%x", rmtMemHndl);

    // Sanity checks
    assert(rmtMemHndl != 0);

    Frame releaseFrame;
    releaseFrame.rmtMemHndl = rmtMemHndl;
    releaseStream_.Write(releaseFrame);
}

} // namespace vpual
