/*
 * PipeCameraBlockOS.cpp
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME PipeCameraBlockOS
#include <mvLog.h>

#include "PipeCameraBlockOS.h"

#include <cassert>

// Defines
// ----------------------------------------------------------------------------------------

// Functions implementation
// ----------------------------------------------------------------------------------------
void PipeCameraBlockOS::Config(Configs * pConfigs, Utils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pConfigs != nullptr);
    assert(pConfigs->pRtRiSrcCtrl != nullptr);
    assert(pConfigs->pRtSeSrcCtrl != nullptr);
    assert(pConfigs->pRtSeEv != nullptr);
    assert(pConfigs->pRtSePreview != nullptr);
    assert(pConfigs->pRtSeVideo != nullptr);

    assert(pUtils != nullptr);
    assert(pUtils->pRefKeeper != nullptr);

    // Copy configs and utils
    configs = *pConfigs;
    utils = *pUtils;

    // Invalidate cache for remote objects
    rmt::utils::cacheInvalidate(configs.pRtRiSrcCtrl, sizeof(*configs.pRtRiSrcCtrl));
    rmt::utils::cacheInvalidate(configs.pRtSeSrcCtrl, sizeof(*configs.pRtSeSrcCtrl));
    rmt::utils::cacheInvalidate(configs.pRtSeEv, sizeof(*configs.pRtSeEv));
    rmt::utils::cacheInvalidate(configs.pRtSePreview, sizeof(*configs.pRtSePreview));
    rmt::utils::cacheInvalidate(configs.pRtSeVideo, sizeof(*configs.pRtSeVideo));
}

void PipeCameraBlockOS::Create()
{
    mvLogLevelSet(MVLOG_WARN);

    pPlgSrcCtrl = PlgSrcCtrl::instance();
    pPlgSrcCtrl->Create(configs.camId);
    osSeSrcCtrl.data.Create(&configs.pRtRiSrcCtrl->data, configs.ipcThreadPrio);
    osRiSrcCtrl.data.Create(&configs.pRtSeSrcCtrl->data, configs.ipcThreadPrio);
    osRiEv.data.Create(&configs.pRtSeEv->data, configs.ipcThreadPrio);
    plgEventsRec.Create(configs.pEventsMesageQueueName);
    plgEventsRec.schParam.sched_priority = configs.ipcThreadPrio;
    osRiPreview.data.Create(&configs.pRtSePreview->data, configs.ipcThreadPrio);
    plgOutPreview.Create(configs.pStreamNamePreview, configs.sizeMaxPreview);
    plgOutPreview.schParam.sched_priority = configs.ipcThreadPrio;
    osRiVideo.data.Create(&configs.pRtSeVideo->data, configs.ipcThreadPrio);
    plgOutVideo.Create(configs.pStreamNameVideo, configs.sizeMaxVideo);
    plgOutVideo.schParam.sched_priority = configs.ipcThreadPrio;
    plgRelPreview.Create(configs.pStreamNameRelPreview, false);
    plgRelPreview.schParam.sched_priority = configs.ipcThreadPrio;
    plgRefKeep.Create(PlgRefKeeper::Mode::KEEP_REF, utils.pRefKeeper, true);
    plgRefKeep.schParam.sched_priority = configs.ipcThreadPrio;
    plgRefRelease.Create(PlgRefKeeper::Mode::RELEASE_REF, utils.pRefKeeper);
    plgRefRelease.schParam.sched_priority = configs.ipcThreadPrio;
}

void PipeCameraBlockOS::Start()
{
    mvLogLevelSet(MVLOG_WARN);

    if(0 == p.Has(pPlgSrcCtrl)) p.Add(pPlgSrcCtrl);
    osSeSrcCtrl.data.AddTo(&p);
    osRiSrcCtrl.data.AddTo(&p);
    osRiEv.data.AddTo(&p);
    p.Add(&plgEventsRec);
    osRiPreview.data.AddTo(&p);
    osRiVideo.data.AddTo(&p);
    p.Add(&plgRefKeep);
    p.Add(&plgRefRelease);
    p.Add(&plgOutPreview);
    p.Add(&plgOutVideo);
    p.Add(&plgRelPreview);

    pPlgSrcCtrl->oCamCtrl[configs.camId].Link(osSeSrcCtrl.data.in);
    osRiSrcCtrl.data.out->Link(&pPlgSrcCtrl->iCamCtrl[configs.camId]);
    osRiEv.data.out->Link(&plgEventsRec.iEv);
    osRiPreview.data.out->Link(&plgRefKeep.in);
    plgRefKeep.out.Link(&plgOutPreview.in);
    plgRelPreview.out.Link(&plgRefRelease.in);
    osRiVideo.data.out->Link(&plgOutVideo.in);

    p.Start();
}

void PipeCameraBlockOS::Stop()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Stop();
    p.Wait();
}

void PipeCameraBlockOS::Destroy()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Delete();
}
