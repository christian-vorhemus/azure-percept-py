/*
 * PipeInferBlockRT.cpp
 *
 *  Created on: Dec 12, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME PipeInferBlockRT
#include <mvLog.h>

#include "PipeInferBlockRT.h"

#include <cassert>

// Defines
// ----------------------------------------------------------------------------------------

// Functions implementation
// ----------------------------------------------------------------------------------------
void PipeInferBlockRT::Config(PipeInferBlockRTCfg * pCfg, PipeInferBlockRTUtils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pCfg != nullptr);
    assert(pCfg->pOsSeIn != nullptr);
    assert(pCfg->pOsRiOut != nullptr);
    assert(pUtils != nullptr);
    assert(pUtils->pFrmPoolAlloc != nullptr);

    // Copy configs and utils
    cfg = *pCfg;
    utils = *pUtils;

    // Invalidate cache for remote objects
    rmt::utils::cacheInvalidate(cfg.pOsSeIn, sizeof(*cfg.pOsSeIn));
    rmt::utils::cacheInvalidate(cfg.pOsRiOut, sizeof(*cfg.pOsRiOut));
}

void PipeInferBlockRT::Create()
{
    mvLogLevelSet(MVLOG_WARN);

    rtRiIn.data.Create(&cfg.pOsSeIn->data, cfg.ipcThreadPrio);
    grpInfer.Create(&cfg.grpInferCfg, utils.pFrmPoolAlloc);
    rtSeOut.data.Create(&cfg.pOsRiOut->data, cfg.ipcThreadPrio);
}

void PipeInferBlockRT::Start()
{
    mvLogLevelSet(MVLOG_WARN);

    rtRiIn.data.AddTo(&p);
    grpInfer.AddTo(&p);
    rtSeOut.data.AddTo(&p);

    rtRiIn.data.out->Link(grpInfer.pIn);
    grpInfer.pOut->Link(rtSeOut.data.in);

    p.Start();
}

void PipeInferBlockRT::Stop()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Stop();
    p.Wait();
}

void PipeInferBlockRT::Destroy()
{
    mvLogLevelSet(MVLOG_WARN);

    grpInfer.Destroy();

    p.Delete();
}
