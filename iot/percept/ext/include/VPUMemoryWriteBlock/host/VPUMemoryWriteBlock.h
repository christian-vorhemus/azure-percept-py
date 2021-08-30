/*
 * VPUMemoryWriteBlock.h
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

#ifndef VPUMEMORYWRITEBLOCK_HOST_VPUMEMORYWRITEBLOCK_H_
#define VPUMEMORYWRITEBLOCK_HOST_VPUMEMORYWRITEBLOCK_H_

// Includes
// ----------------------------------------------------------------------------
#include <XLink.h>
#include <VpualDispatcher.h>

#include <VPUBlockTypes.h>
#include <VPUBlockXLink.h>

// Defines
// ----------------------------------------------------------------------------
namespace vpual {

class MemoryWriteBlock : public core::Stub {
public:
    MemoryWriteBlock();
    ~MemoryWriteBlock();
    RmtMemHndl Write(const Frame & frame);
    void Release(const RmtMemHndl hndl);

private:
    XLinkVpuIn inStream_;
    XLinkVpuOut outStream_;
    XLinkVpuIn releaseStream_;
};

} // namespace vpual

#endif /* VPUMEMORYWRITEBLOCK_HOST_VPUMEMORYWRITEBLOCK_H_ */
