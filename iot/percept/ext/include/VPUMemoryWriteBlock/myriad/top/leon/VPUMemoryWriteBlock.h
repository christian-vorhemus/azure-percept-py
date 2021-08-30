/*
 * VPUMemoryWriteBlock.h
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

#ifndef VPUMEMORYWRITEBLOCK_H_
#define VPUMEMORYWRITEBLOCK_H_

// Includes
//-----------------------------------------------------------------------------
#include <Decoder.h>
#include <RmtUtilsCache.h>

#include <PipeMemoryWriteBlock.h>

// Defines
//-----------------------------------------------------------------------------
namespace vpual {

class MemoryWriteBlock final : public core::Decoder {
public:
    struct Utils {
        IAllocator * pFrmPoolAlloc;
        RefKeeper * pRefKeeper;
    };

    MemoryWriteBlock() = delete;
    MemoryWriteBlock(const Utils & utils);
    ~MemoryWriteBlock();
    void Decode(core::Message *request, core::Message *response) override;

private:
    Utils utils_;
    rmt::utils::CacheAligned<PipeMemoryWriteBlock> pipe_;
};

} // namespace vpual

#endif /* VPUMEMORYWRITEBLOCK_H_ */
