/*
 * PipeMemoryWriteBlock.h
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

#ifndef PIPEMEMORYWRITEBLOCK_H_
#define PIPEMEMORYWRITEBLOCK_H_

// Includes
// ----------------------------------------------------------------------------------------
#include <Flic.h>
#include <GrupsTypes.h>

#include <PlgXlinkIn.hpp>
#include <PlgRefKeeper.h>
#include <PlgXlinkOut.hpp>
#include <IFlicPipeWrap.h>
#include <UnifiedFlicMsg.h>

// Defines
// ----------------------------------------------------------------------------------------

// Class definition
// ----------------------------------------------------------------------------------------
class PipeMemoryWriteBlock : public IFlicPipeWrap
{
public:
    struct Configs {
        const char * pInStreamName;
        const char * pOutStreamName;
        std::uint32_t outSizeMax;
        const char * pReleaseStreamName;
    };

    struct Utils {
        IAllocator * pFrmPoolAlloc;
        RefKeeper * pRefKeeper;
    };

    Pipeline p;

    PlgXlinkIn plgIn;
    PlgRefKeeper plgRefKeep;
    PlgXlinkOut plgOut;
    PlgXlinkIn plgRelease;
    PlgRefKeeper plgRefRelease;

    Configs configs;
    Utils utils;

    void Config(Configs * pConfigs, Utils * pUtils);
    void Create();
    void Start();
    void Stop();
    void Destroy();
};

#endif /* PIPEMEMORYWRITEBLOCK_H_ */
