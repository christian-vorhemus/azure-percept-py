// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#ifndef _XLINKSTREAM_H
#define _XLINKSTREAM_H

#include "XLinkPublicDefines.h"

# if (defined(_WIN32) || defined(_WIN64))
#  include "win_semaphore.h"
# else
#  ifdef __APPLE__
#   include "pthread_semaphore.h"
#  else
#   include <semaphore.h>
# endif
# endif

/**
 * @brief Streams opened to device
 */
typedef struct{
    char name[MAX_STREAM_NAME_LENGTH];
    streamId_t id;
    uint32_t writeSize;
    uint32_t readSize;  /*No need of read buffer. It's on remote,
    will read it directly to the requested buffer*/
    streamPacketDesc_t packets[XLINK_MAX_PACKETS_PER_STREAM];
    uint32_t availablePackets;
    uint32_t blockedPackets;

    uint32_t firstPacket;
    uint32_t firstPacketUnused;
    uint32_t firstPacketFree;

    uint32_t remoteFillLevel;
    uint32_t localFillLevel;
    uint32_t remoteFillPacketLevel;

    uint32_t closeStreamInitiated;

    sem_t sem;
}streamDesc_t;

XLinkError_t XLinkStreamInitialize(
    streamDesc_t* stream, streamId_t id, const char* name);

void XLinkStreamReset(streamDesc_t* stream);

#endif //_XLINKSTREAM_H
