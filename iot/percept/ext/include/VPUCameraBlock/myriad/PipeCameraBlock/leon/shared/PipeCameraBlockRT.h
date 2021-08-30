/*
 * PipeCameraBlockRT.h
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

#ifndef PIPECAMERABLOCKRT_H_
#define PIPECAMERABLOCKRT_H_

// Includes
// ----------------------------------------------------------------------------------------
#include <Resources.h>
#include <CmdGenMsg.h>
#include "GrupsTypes.h"
#include <PlgSrcMipi.h>
#include <PlgIspCtrl.h>
#include <PlgColorIsp1xPolyStrideAlign.h>
#include <PlgColorIspScaleCrop.h>
#include <plgppenc.h>
#include <PlgCustVidEncCtrl.h>
#include <GrpVidEnc.h>
#include <PlgVidEnc.h>
#include <Pool.h>
#include <PlgCamCtrl.h>
#include <GrpBridgeLOS2LRT_RT.h>
#include <GrpBridgeLRT2LOS_RT.h>
#include <IFlicPipeWrap.h>
#include <RmtUtilsCache.h>
#include <UnifiedFlicMsg.h>

// Defines
// ----------------------------------------------------------------------------------------

// Class definition
// ----------------------------------------------------------------------------------------
class PipeCameraBlockRT : public IFlicPipeWrap
{
public:
    struct Configs {
        enum class ColorIspType {
            Native,
            ScaleCrop
        };

        struct ScaleCrop {
            YuvScaleFactors scaleFactors;
            icRect cropWindow;
        };

        std::uint8_t camId;
        SpSrcCfg srcCfg;
        int ipcThreadPrio;
        rmt::utils::CacheAligned<GrpBridgeLOS2LRT_OS<CmdGenMsg>> * pOsSeSrcCtrl;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<CmdGenMsg>> * pOsRiSrcCtrl;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<CmdGenMsg>> * pOsRiEv;
        SpIspCfg ispCfg;
        ColorIspType colorIspType;
        ScaleCrop scaleCrop;
        PpEncCfgT postProcCfg;
        ResourcesAlloc postProcRes;
        PoolConfig poolCfgPreview;
        bool enableVideo;
        PoolConfig poolCfgVideo;
        PoolConfig poolCfgStill;
        std::uint32_t nominalFps;
        GrpVidEnc::Configs grpVidEncCfg;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> * pOsRiPreview;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> * pOsRiVideo;
    };

    struct Utils {
        IAllocator * pFrmPoolAlloc;
    };

    Pipeline p;

    PlgSrcMipi plgSrc;
    PlgPool<ImgFrame> plgPoolSrc;
    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_RT<CmdGenMsg>> rtRiSrcCtrl;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<CmdGenMsg>> rtSeSrcCtrl;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<CmdGenMsg>> rtSeEv;
    PlgIspCtrl plgIspCtrl;
    PlgColorIsp1xPolyStrideAlign plgIsp1xPoly;
    PlgColorIspScaleCrop plgIspScaleCrop;
    PlgPool<ImgFrame> plgPoolIsp;
    PlgPpEnc plgIspPostProc;
    PlgPool<ImgFrame> plgPoolPPPreview;
    PlgPool<ImgFrame> plgPoolPPVideo;
    PlgCustVidEncCtrl plgCustEncCtrl;
    GrpVidEnc grpVidEnc;
    PlgCamCtrl plgCamCtrl;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> rtSePreview;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> rtSeVideo;

    Configs configs;
    Utils utils;

    void Config(Configs * pConfigs, Utils * pUtils);
    void Create();
    void Start();
    void Stop();
    void Destroy();
};

#endif /* PIPECAMERABLOCKRT_H_ */
