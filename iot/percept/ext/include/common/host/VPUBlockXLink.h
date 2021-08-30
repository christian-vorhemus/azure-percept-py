/*
 * VPUBlockXLink.h
 *
 *  Created on: Mar 20, 2021
 *      Author: apalfi
 */

#ifndef VPUBLOCKXLINK_H_
#define VPUBLOCKXLINK_H_

// Includes
// ----------------------------------------------------------------------------
#include <deque>
#include <string>
#include <XLink.h>
#include <VPUBlockTypes.h>

// Defines
// ----------------------------------------------------------------------------
namespace vpual {

// Defines
// ----------------------------------------------------------------------------
class XLink
{
public:
    std::string streamName;
    streamId_t streamId = 0;

    void Open(linkId_t linkId,
              const char * pStreamName,
              std::uint32_t size);
    void Read(Frame * pFrame);
    void Read(const std::deque<BufferSegment> & segments,
              std::deque<Frame> * pFrames);
    void Write(const Frame & pFrame);
};

class XLinkVpuIn : public XLink
{
public:
    void Open(linkId_t linkId,
              const char * pStreamName,
              std::uint32_t size = 0);
};

class XLinkVpuOut : public XLink
{
public:
    void Open(linkId_t linkId,
              const char * pStreamName);
};

// Exported functions
// ----------------------------------------------------------------------------
void XLinkReadHeader(streamId_t streamId,
                     const char * streamName,
                     Frame * pFrame,
                     std::uint32_t * pPayloadSize);

void XLinkRead(const streamId_t streamId,
               const char * streamName,
               Frame * pFrame);

void XLinkRead(const streamId_t streamId,
               const char * streamName,
               const std::deque<BufferSegment> & segments,
               std::deque<Frame> * pFrames);

void XLinkWrite(const streamId_t streamId,
                const char * streamName,
                const Frame & pFrame);

} // namespace vpual

#endif /* VPUBLOCKXLINK_H_ */
