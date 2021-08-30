/*
 * PipeInferBlockOS.h
 *
 *  Created on: Dec 12, 2020
 *      Author: apalfi
 */

#ifndef PIPEINFERBLOCKOS_H_
#define PIPEINFERBLOCKOS_H_

// Includes
// ----------------------------------------------------------------------------------------
#include <Flic.h>
#include <GrupsTypes.h>

#include <PlgXlinkIn.hpp>
#include <PlgRefKeeper.h>
#include <GrpBridgeLOS2LRT_OS.h>
#include <GrpBridgeLRT2LOS_OS.h>
#include <PlgXlinkOut.hpp>
#include <RmtUtilsCache.h>
#include <IFlicPipeWrap.h>
#include <UnifiedFlicMsg.h>

// Defines
// ----------------------------------------------------------------------------------------
typedef struct {
    const char * pInChanName;
    int ipcThreadPrio;
    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_RT<UnifiedMsg>> * pRtRiIn;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> * pRtSeOut;
    const char * pOutChanName;
    uint32_t outSizeMax;
} PipeInferBlockOSCfg;

typedef struct {
    RefKeeper * pRefKeeper;
} PipeInferBlockOSUtils;

// Class definition
// ----------------------------------------------------------------------------------------
class PipeInferBlockOS : public IFlicPipeWrap
{
public:
    Pipeline p;

    PlgXlinkIn plgXlinkIn;
    PlgRefKeeper plgRefKeeper;
    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_OS<UnifiedMsg>> osSeIn;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> osRiOut;
    PlgXlinkOut plgXlinkOut;

    PipeInferBlockOSCfg cfg;
    PipeInferBlockOSUtils utils;

    void Config(PipeInferBlockOSCfg * pCfg, PipeInferBlockOSUtils * pUtils);
    void Create();
    void Start();
    void Stop();
    void Destroy();
};

#endif /* PIPEINFERBLOCKOS_H_ */
