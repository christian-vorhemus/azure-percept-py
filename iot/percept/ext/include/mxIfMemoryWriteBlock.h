/*
 * mxIfMemoryWriteBlock.h
 *
 *  Created on: Feb 5, 2021
 *      Author: apalfi
 */

#ifndef MXIFMEMORYWRITEBLOCK_H_
#define MXIFMEMORYWRITEBLOCK_H_

// Includes
// -------------------------------------------------------------------------------------
#include <memory>

#include "mxIfMemoryHandle.h"

// Classes
// -------------------------------------------------------------------------------------
namespace mxIf
{
class MemoryWriteBlock
{
public:
    MemoryWriteBlock();
    ~MemoryWriteBlock();
    MemoryHandle Write(MemoryHandle);
    void Release(MemoryHandle);

private:
    struct Private;
    std::unique_ptr<Private> private_;
};
} // namespace mxIf

#endif /* MXIFMEMORYWRITEBLOCK_H_ */
