/*
 * PipeInferBlockRT.h
 *
 *  Created on: Dec 12, 2020
 *      Author: apalfi
 */

#ifndef PIPEINFERBLOCKRT_H_
#define PIPEINFERBLOCKRT_H_

// Includes
// ----------------------------------------------------------------------------------------
#include <GrpBridgeLOS2LRT_RT.h>
#include <GrpBridgeLRT2LOS_RT.h>
#include <GrpInferGeneric.h>
#include <IFlicPipeWrap.h>
#include <RmtUtilsCache.h>

#include <UnifiedFlicMsg.h>

// Defines
// ----------------------------------------------------------------------------------------
typedef struct {
    int ipcThreadPrio;
    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_OS<UnifiedMsg>> * pOsSeIn;
    GrpInferGenericCfg grpInferCfg;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> * pOsRiOut;
} PipeInferBlockRTCfg;

typedef struct {
    IAllocator * pFrmPoolAlloc;
} PipeInferBlockRTUtils;

// Class definition
// ----------------------------------------------------------------------------------------
class PipeInferBlockRT : public IFlicPipeWrap
{
public:
    Pipeline p;

    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_RT<UnifiedMsg>> rtRiIn;
    GrpInferGeneric grpInfer;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> rtSeOut;

    PipeInferBlockRTCfg cfg;
    PipeInferBlockRTUtils utils;

    void Config(PipeInferBlockRTCfg * pCfg, PipeInferBlockRTUtils * pUtils);
    void Create();
    void Start();
    void Stop();
    void Destroy();
};

#endif /* PIPEINFERBLOCKRT_H_ */
