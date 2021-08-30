/*
 * PipeCameraBlockRT.cpp
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME PipeCameraBlockRT
#include <mvLog.h>

#include "PipeCameraBlockRT.h"

#include <cassert>

// Defines
// ----------------------------------------------------------------------------------------
#define EXPOSURE_TIME_HEADROOM_US 1000

// Functions implementation
// ----------------------------------------------------------------------------------------
void PipeCameraBlockRT::Config(Configs * pConfigs, Utils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pConfigs != nullptr);
    assert(pConfigs->pOsSeSrcCtrl != nullptr);
    assert(pConfigs->pOsRiSrcCtrl != nullptr);
    assert(pConfigs->pOsRiEv != nullptr);
    assert(pConfigs->pOsRiPreview != nullptr);
    assert(pConfigs->pOsRiVideo != nullptr);
    assert(pUtils != nullptr);
    assert(pUtils->pFrmPoolAlloc != nullptr);

    // Copy configs and utils
    configs = *pConfigs;
    utils = *pUtils;

    // Invalidate cache for remote objects
    rmt::utils::cacheInvalidate(configs.pOsSeSrcCtrl, sizeof(*configs.pOsSeSrcCtrl));
    rmt::utils::cacheInvalidate(configs.pOsRiSrcCtrl, sizeof(*configs.pOsRiSrcCtrl));
    rmt::utils::cacheInvalidate(configs.pOsRiEv, sizeof(*configs.pOsRiEv));
    rmt::utils::cacheInvalidate(configs.pOsRiPreview, sizeof(*configs.pOsRiPreview));
    rmt::utils::cacheInvalidate(configs.pOsRiVideo, sizeof(*configs.pOsRiVideo));
}

void PipeCameraBlockRT::Create()
{
    mvLogLevelSet(MVLOG_WARN);

    plgSrc.Create((icSourceInstance)configs.camId, MIPI_DATA_PACKED);
    plgPoolSrc.Create(utils.pFrmPoolAlloc,
                      configs.srcCfg.nrFrmsSrc,
                      configs.srcCfg.maxImgSz,
                      false);
    rtRiSrcCtrl.data.Create(&configs.pOsSeSrcCtrl->data, configs.ipcThreadPrio);
    rtSeSrcCtrl.data.Create(&configs.pOsRiSrcCtrl->data, configs.ipcThreadPrio);
    rtSeEv.data.Create(&configs.pOsRiEv->data, configs.ipcThreadPrio);
    plgIspCtrl.Create();
    switch(configs.colorIspType)
    {
        default:
        case Configs::ColorIspType::Native:
            plgIsp1xPoly.Create(configs.camId,
                                configs.ispCfg.firstCmxSliceUsed,
                                configs.ispCfg.nrOfCmxSliceUsed,
                                configs.ispCfg.lineStrideAlign,
                                1);
            break;

        case Configs::ColorIspType::ScaleCrop:
            plgIspScaleCrop.Create(configs.camId,
                                   configs.ispCfg.firstCmxSliceUsed,
                                   configs.ispCfg.nrOfCmxSliceUsed,
                                   configs.ispCfg.lineStrideAlign,
                                   &configs.scaleCrop.scaleFactors,
                                   &configs.scaleCrop.cropWindow,
                                   1);
            break;
    }
    plgPoolIsp.Create(utils.pFrmPoolAlloc,
                      configs.ispCfg.nrFrmsIsp,
                      configs.ispCfg.maxImgSz,
                      false);
    plgIspPostProc.Create(configs.camId,
                          &configs.postProcCfg,
                          1 /* input buffer size */,
                          &configs.postProcRes);
    plgPoolPPPreview.Create(utils.pFrmPoolAlloc,
                            configs.poolCfgPreview.slotsNr,
                            configs.poolCfgPreview.slotSz,
                            true);
    plgPoolPPVideo.Create(utils.pFrmPoolAlloc,
                          configs.poolCfgVideo.slotsNr,
                          configs.poolCfgVideo.slotSz,
                          false);
    PlgCustVidEncCtrl::Config custVidEncCtrlCfg;
    // set frame duration threshold to 33ms like in virt_cm_sensor_exp_gain() from virt_cm.c in Guzzi
    custVidEncCtrlCfg.frame_duration_threshold_us = 1000000/configs.nominalFps - EXPOSURE_TIME_HEADROOM_US;
    plgCustEncCtrl.Create(&custVidEncCtrlCfg);
    GrpVidEnc::Utils grpVidEncUtils =
    {
            .pFrmPoolAlloc = utils.pFrmPoolAlloc
    };
    grpVidEnc.Create(&configs.grpVidEncCfg, &grpVidEncUtils);
    plgCamCtrl.Create();
    plgCamCtrl.schParam.sched_priority = configs.ipcThreadPrio;
    rtSePreview.data.Create(&configs.pOsRiPreview->data, configs.ipcThreadPrio);
    rtSeVideo.data.Create(&configs.pOsRiVideo->data, configs.ipcThreadPrio);
}

void PipeCameraBlockRT::Start()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Add(&plgSrc);
    p.Add(&plgPoolSrc);
    rtRiSrcCtrl.data.AddTo(&p);
    rtSeSrcCtrl.data.AddTo(&p);
    rtSeEv.data.AddTo(&p);
    p.Add(&plgIspCtrl);
    switch(configs.colorIspType)
    {
        default:
        case Configs::ColorIspType::Native:
            p.Add(&plgIsp1xPoly);
            break;

        case Configs::ColorIspType::ScaleCrop:
            p.Add(&plgIspScaleCrop);
            break;
    }
    p.Add(&plgPoolIsp);
    p.Add(&plgIspPostProc);
    p.Add(&plgPoolPPPreview);
    p.Add(&plgPoolPPVideo);
    p.Add(&plgCustEncCtrl);
    grpVidEnc.AddTo(&p);
    p.Add(&plgCamCtrl);
    rtSePreview.data.AddTo(&p);
    rtSeVideo.data.AddTo(&p);

    rtRiSrcCtrl.data.out->Link(&plgSrc.iCtrl);
    plgSrc.oCtrl.Link(rtSeSrcCtrl.data.in);
    plgSrc.oSoF.Link(rtSeEv.data.in);
    plgSrc.oEoF.Link(rtSeEv.data.in);
    plgSrc.oLineHit.Link(rtSeEv.data.in);
    plgSrc.oErr.Link(rtSeEv.data.in);
    plgPoolSrc.out.Link(&plgSrc.iFrm);
    plgSrc.oFrm.Link(&plgIspCtrl.inO);
    switch(configs.colorIspType)
    {
        default:
        case Configs::ColorIspType::Native:
            plgPoolIsp.out.Link(&plgIsp1xPoly.inO);
            plgIspCtrl.out.Link(&plgIsp1xPoly.inI);
            plgIsp1xPoly.outE.Link(rtSeEv.data.in);
            plgIsp1xPoly.outF.Link(&plgIspPostProc.i_fr_);
            break;

        case Configs::ColorIspType::ScaleCrop:
            plgPoolIsp.out.Link(&plgIspScaleCrop.inO);
            plgIspCtrl.out.Link(&plgIspScaleCrop.inI);
            plgIspScaleCrop.outE.Link(rtSeEv.data.in);
            plgIspScaleCrop.outF.Link(&plgIspPostProc.i_fr_);
            break;
    }
    plgPoolPPPreview.out.Link(&plgIspPostProc.i_fr_prv_);
    plgPoolPPVideo.out.Link(&plgIspPostProc.i_fr_vdo_);
    plgIspPostProc.o_fr_prv_.Link(&plgCamCtrl.fromPpEncBGR);
    if (configs.enableVideo) {
        plgIspPostProc.o_fr_vdo_.Link(&plgCustEncCtrl.in);
        plgCustEncCtrl.out.Link(grpVidEnc.pIn);
    }
    grpVidEnc.pOut->Link(&plgCamCtrl.fromVidEnc);
    plgCamCtrl.outBGR.Link(rtSePreview.data.in);
    plgCamCtrl.outEnc.Link(rtSeVideo.data.in);

    p.Start();
}

void PipeCameraBlockRT::Stop()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Stop();
    p.Wait();
}

void PipeCameraBlockRT::Destroy()
{
    mvLogLevelSet(MVLOG_WARN);

    grpVidEnc.Destroy();

    p.Delete();
}
