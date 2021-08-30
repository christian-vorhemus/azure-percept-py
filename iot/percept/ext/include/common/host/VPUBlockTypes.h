/*
 * VPUBlockTypes.h
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

#ifndef COMMON_HOST_VPUBLOCKTYPES_H_
#define COMMON_HOST_VPUBLOCKTYPES_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <swcFrameTypes.h>

// Defines
// ----------------------------------------------------------------------------
namespace vpual {

struct Buffer {
    void* base;
    std::uint32_t size;

    Buffer():
        base(nullptr),
        size(0)
    {}
};

struct BufferSegment {
    std::uint32_t offset;
    std::uint32_t size;

    BufferSegment():
        offset(0),
        size(0)
    {}
};

struct FrameMeta {
    frameType type;
    std::uint32_t width;
    std::uint32_t height;
    std::int64_t seqNo;
    std::int64_t ts;

    FrameMeta():
        type(BITSTREAM),
        width(0),
        height(0),
        seqNo(0),
        ts(0)
    {}
};

struct Roi {
    std::int32_t x1;
    std::int32_t y1;
    std::int32_t x2;
    std::int32_t y2;

    Roi():
        x1(0),
        y1(0),
        x2(0),
        y2(0)
    {}
};

using RmtMemHndl = std::uint32_t;

struct Frame {
    Buffer buffer;
    RmtMemHndl rmtMemHndl;
    FrameMeta meta;
    Roi roi;
};

} // namespace vpual

#endif /* COMMON_HOST_VPUBLOCKTYPES_H_ */
