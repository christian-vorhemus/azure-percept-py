/*
 * mxIfMemoryHandle.h
 *
 *  Created on: Apr 6, 2020
 *      Author: apalfi
 */

#ifndef HOST_MXIF_MXIFMEMORYHANDLE_H_
#define HOST_MXIF_MXIFMEMORYHANDLE_H_

// Includes
// -------------------------------------------------------------------------------------
#include <stdint.h>

// Classes
// -------------------------------------------------------------------------------------
namespace mxIf
{
    class MemoryHandle
    {
    public:
        enum class Types
        {
            LocalMem,
            RemoteMem,
        };

        enum class Formats {
            None,
            BGRp,
        };

        Types type;
        void *pBuf;
        uint32_t bufSize;
        uint32_t rmtMemHndl;
        int64_t seqNo;
        int64_t ts;
        uint32_t width;
        uint32_t height;
        Formats format;

        MemoryHandle();
        void TransferTo(void* pDest);
    };
} // namespace mxIf

#endif /* HOST_MXIF_MXIFMEMORYHANDLE_H_ */
