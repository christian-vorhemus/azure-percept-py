/*
 * PipeInferBlockOS.cpp
 *
 *  Created on: Dec 12, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME PipeInferBlockOS
#include <mvLog.h>

#include "PipeInferBlockOS.h"

#include <cassert>

// Defines
// ----------------------------------------------------------------------------------------

// Functions implementation
// ----------------------------------------------------------------------------------------
void PipeInferBlockOS::Config(PipeInferBlockOSCfg * pCfg, PipeInferBlockOSUtils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pCfg != nullptr);
    assert(pCfg->pRtRiIn != nullptr);
    assert(pCfg->pRtSeOut != nullptr);
    assert(pUtils != nullptr);
    assert(pUtils->pRefKeeper != nullptr);

    // Copy configs and utils
    cfg = *pCfg;
    utils = *pUtils;

    // Invalidate cache for remote objects
    rmt::utils::cacheInvalidate(cfg.pRtRiIn, sizeof(*cfg.pRtRiIn));
    rmt::utils::cacheInvalidate(cfg.pRtSeOut, sizeof(*cfg.pRtSeOut));
}

void PipeInferBlockOS::Create()
{
    mvLogLevelSet(MVLOG_WARN);

    plgXlinkIn.Create(cfg.pInChanName, true);
    plgXlinkIn.schParam.sched_priority = cfg.ipcThreadPrio;
    plgRefKeeper.Create(PlgRefKeeper::Mode::GET_REF, utils.pRefKeeper);
    osSeIn.data.Create(&cfg.pRtRiIn->data, cfg.ipcThreadPrio);
    osRiOut.data.Create(&cfg.pRtSeOut->data, cfg.ipcThreadPrio);
    plgXlinkOut.Create(cfg.pOutChanName, cfg.outSizeMax);
    plgXlinkOut.schParam.sched_priority = cfg.ipcThreadPrio;
}

void PipeInferBlockOS::Start()
{
    mvLogLevelSet(MVLOG_WARN);

    plgXlinkIn.AddTo(&p);
    p.Add(&plgRefKeeper);
    osSeIn.data.AddTo(&p);
    osRiOut.data.AddTo(&p);
    p.Add(&plgXlinkOut);

    plgXlinkIn.out.Link(&plgRefKeeper.in);
    plgRefKeeper.out.Link(osSeIn.data.in);
    osRiOut.data.out->Link(&plgXlinkOut.in);

    p.Start();
}

void PipeInferBlockOS::Stop()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Stop();
    p.Wait();
}

void PipeInferBlockOS::Destroy()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Delete();
}
