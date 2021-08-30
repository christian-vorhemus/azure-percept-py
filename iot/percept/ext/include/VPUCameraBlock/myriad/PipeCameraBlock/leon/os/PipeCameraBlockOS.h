/*
 * PipeCameraBlockOS.h
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

#ifndef PIPECAMERABLOCKOS_H_
#define PIPECAMERABLOCKOS_H_

// Includes
// ----------------------------------------------------------------------------------------
#include <Flic.h>
#include <FlicRmt.h>
#include <CmdGenMsg.h>
#include <GrupsTypes.h>

#include <PlgSrcCtrl.h>
#include <GrpBridgeLOS2LRT_OS.h>
#include <GrpBridgeLRT2LOS_OS.h>
#include <PlgEventsRec.h>
#include <PlgXlinkIn.hpp>
#include <PlgXlinkOut.hpp>
#include <PlgRefKeeper.h>
#include <RmtUtilsCache.h>
#include <IFlicPipeWrap.h>
#include <UnifiedFlicMsg.h>

// Defines
// ----------------------------------------------------------------------------------------

// Class definition
// ----------------------------------------------------------------------------------------
class PipeCameraBlockOS : public IFlicPipeWrap
{
public:
    struct Configs {
        std::uint8_t camId;
        int ipcThreadPrio;
        rmt::utils::CacheAligned<GrpBridgeLOS2LRT_RT<CmdGenMsg>> * pRtRiSrcCtrl;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<CmdGenMsg>> * pRtSeSrcCtrl;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<CmdGenMsg>> * pRtSeEv;
        const char* pEventsMesageQueueName;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> * pRtSePreview;
        const char * pStreamNamePreview;
        std::uint32_t sizeMaxPreview;
        rmt::utils::CacheAligned<GrpBridgeLRT2LOS_RT<UnifiedMsg>> * pRtSeVideo;
        const char * pStreamNameVideo;
        std::uint32_t sizeMaxVideo;
        const char * pStreamNameRelPreview;
    };

    struct Utils {
        RefKeeper * pRefKeeper;
    };

    Pipeline p;

    PlgSrcCtrl * pPlgSrcCtrl;
    rmt::utils::CacheAligned<GrpBridgeLOS2LRT_OS<CmdGenMsg>> osSeSrcCtrl;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<CmdGenMsg>> osRiSrcCtrl;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<CmdGenMsg>> osRiEv;
    PlgEventsRec plgEventsRec;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> osRiPreview;
    PlgXlinkOut plgOutPreview;
    rmt::utils::CacheAligned<GrpBridgeLRT2LOS_OS<UnifiedMsg>> osRiVideo;
    PlgXlinkOut plgOutVideo;
    PlgXlinkIn plgRelPreview;
    PlgRefKeeper plgRefKeep;
    PlgRefKeeper plgRefRelease;

    Configs configs;
    Utils utils;

    void Config(Configs * pConfigs, Utils * pUtils);
    void Create();
    void Start();
    void Stop();
    void Destroy();
};

#endif /* PIPECAMERABLOCKOS_H_ */
