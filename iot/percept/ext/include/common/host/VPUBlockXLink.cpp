/*
 * VPUBlockXLink.cpp
 *
 *  Created on: Mar 20, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME VPUBlockXLink
#include <mvLog.h>

#include "VPUBlockXLink.h"

#include <cassert>
#include <cstdlib>
#include <string.h>
#include <secure_functions.h>

#include <PlgXlinkShared.h>
#include <VPUBlockTypes.h>

// Defines
// ----------------------------------------------------------------------------
namespace vpual {

// Functions implementation
// ----------------------------------------------------------------------------
void XLink::Open(linkId_t linkId,
                 const char * pStreamName,
                 std::uint32_t size)
{
    mvLogLevelSet(MVLOG_WARN);

    streamName = pStreamName;

    streamId = XLinkOpenStream(linkId, streamName.c_str(), size);
    if ((streamId == INVALID_STREAM_ID) || (streamId == INVALID_STREAM_ID_OUT_OF_MEMORY))
    {
        mvLog(MVLOG_FATAL, "[%s] Could not open XLink stream\n", streamName.c_str());
        exit(1);
    }
    mvLog(MVLOG_INFO,"[%s] Opened XLink stream", streamName.c_str());
}

void XLink::Read(Frame * pFrame)
{
    XLinkRead(streamId, streamName.c_str(), pFrame);
}

void XLink::Read(const std::deque<BufferSegment> & segments,
                 std::deque<Frame> * pFrames)
{
    XLinkRead(streamId, streamName.c_str(), segments, pFrames);
}

void XLink::Write(const Frame & frame)
{
    XLinkWrite(streamId, streamName.c_str(), frame);
}

void XLinkVpuIn::Open(linkId_t linkId,
                      const char * pStreamName,
                      std::uint32_t size)
{
    // Reserve space for at least 2 headers
    size += 2 * sizeof(XLinkHeader);
    XLink::Open(linkId, pStreamName, size);
}

void XLinkVpuOut::Open(linkId_t linkId,
                       const char * pStreamName)
{
    // Use size "1" for blocking call
    XLink::Open(linkId, pStreamName, 1);
}

void XLinkReadHeader(streamId_t streamId,
                     const char * streamName,
                     Frame * pFrame,
                     std::uint32_t * pPayloadSize)
{
    streamPacketDesc_t * packet = nullptr;
    XLinkError_t status = X_LINK_SUCCESS;

    // Sanity checks
    assert(pFrame != nullptr);
    assert(pPayloadSize != nullptr);

    // Read header
    status = XLinkReadData(streamId, &packet);
    assert(status == X_LINK_SUCCESS);
    assert(packet && packet->length == sizeof(XLinkHeader));

    // Parse header
    XLinkHeader * pHeader = reinterpret_cast<XLinkHeader *>(packet->data);

    pFrame->buffer.base = nullptr;
    pFrame->buffer.size = 0;
    pFrame->rmtMemHndl = pHeader->rmtMemHndl;
    pFrame->meta.type = static_cast<frameType>(pHeader->meta.type);
    pFrame->meta.width = pHeader->meta.width;
    pFrame->meta.height = pHeader->meta.height;
    pFrame->meta.seqNo = pHeader->meta.seqNo;
    pFrame->meta.ts = pHeader->meta.ts;
    pFrame->roi.x1 = pHeader->roi.x1;
    pFrame->roi.y1 = pHeader->roi.y1;
    pFrame->roi.x2 = pHeader->roi.x2;
    pFrame->roi.y2 = pHeader->roi.y2;
    *pPayloadSize = pHeader->payloadSize;
    status = XLinkReleaseData(streamId);
    assert(status == X_LINK_SUCCESS);

    mvLog(MVLOG_DEBUG,"[%s] *pPayloadSize = %u", streamName, *pPayloadSize);
}

void XLinkRead(streamId_t streamId,
               const char * streamName,
               Frame * pFrame)
{
    std::uint32_t payloadSize = 0;

    // Sanity checks
    assert(pFrame != nullptr);

    // Read header
    XLinkReadHeader(streamId, streamName, pFrame, &payloadSize);

    // Check if payload will follow
    if (payloadSize == 0)
        return;

    // Read payload
    streamPacketDesc_t * packet = nullptr;
    XLinkError_t status = X_LINK_SUCCESS;

    status = XLinkReadData(streamId, &packet);
    assert(status == X_LINK_SUCCESS);
    assert(packet != nullptr);
    assert(packet->length == payloadSize);

    // Allocate memory
    pFrame->buffer.base = malloc(packet->length);
    assert(pFrame->buffer.base != nullptr);
    pFrame->buffer.size = packet->length;

    memcpy_s(pFrame->buffer.base, pFrame->buffer.size, packet->data, packet->length);

    status = XLinkReleaseData(streamId);
    assert(status == X_LINK_SUCCESS);
}

void XLinkRead(const streamId_t streamId,
               const char * streamName,
               const std::deque<BufferSegment> & segments,
               std::deque<Frame> * pFrames)
{
    std::uint32_t payloadSize = 0;
    Frame frame {};

    // Sanity checks
    assert(pFrames != nullptr);
    assert(pFrames->size() == 0);

    // Read header
    XLinkReadHeader(streamId, streamName, &frame, &payloadSize);

    // Check if payload will follow
    if (payloadSize == 0)
        return;

    // Read payload
    streamPacketDesc_t * packet = nullptr;
    XLinkError_t status = X_LINK_SUCCESS;

    status = XLinkReadData(streamId, &packet);
    assert(status == X_LINK_SUCCESS);
    assert(packet != nullptr);
    assert(packet->length == payloadSize);

    // Copy every segment to a separate Frame
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        mvLog(MVLOG_DEBUG,"[%s] segments[i].offset = %u, segments[i].size = %u", streamName, segments[i].offset, segments[i].size);

        // Sanity checks
        assert(packet->length >= segments[i].offset);
        assert(packet->length >= (segments[i].offset + segments[i].size));

        // Allocate memory
        frame.buffer.base = malloc(segments[i].size);
        assert(frame.buffer.base != nullptr);
        frame.buffer.size = segments[i].size;

        // Copy payload segment
        memcpy_s(frame.buffer.base, frame.buffer.size, (std::uint8_t*)packet->data + segments[i].offset, segments[i].size);

        // Add to list of frames
        pFrames->push_back(frame);
    }

    status = XLinkReleaseData(streamId);
    assert(status == X_LINK_SUCCESS);
}

void XLinkWrite(streamId_t streamId,
                const char * streamName,
                const Frame & frame)
{
    XLinkError_t status = X_LINK_SUCCESS;

    // Prepare header
    XLinkHeader header = {};
    header.rmtMemHndl = frame.rmtMemHndl;
    header.meta.type = frame.meta.type;
    header.meta.width = frame.meta.width;
    header.meta.height = frame.meta.height;
    header.meta.seqNo = frame.meta.seqNo;
    header.meta.ts = frame.meta.ts;
    header.roi.x1 = frame.roi.x1;
    header.roi.y1 = frame.roi.y1;
    header.roi.x2 = frame.roi.x2;
    header.roi.y2 = frame.roi.y2;
    header.payloadSize = frame.buffer.size;

    // Send header
    status = XLinkWriteData(streamId, (const uint8_t*)&header, sizeof(header));
    assert(status == X_LINK_SUCCESS);

    // Check if payload will follow
    if (frame.buffer.base == nullptr)
        return;

    // Send payload
    assert(frame.buffer.size != 0);

    status = XLinkWriteData(streamId, (const uint8_t*)frame.buffer.base, frame.buffer.size);
    assert(status == X_LINK_SUCCESS);
}

} // namespace vpual
