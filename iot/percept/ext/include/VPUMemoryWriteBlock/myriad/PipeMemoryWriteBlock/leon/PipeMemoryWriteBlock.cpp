/*
 * PipeMemoryWriteBlock.cpp
 *
 *  Created on: Feb 7, 2021
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME PipeMemoryWriteBlock
#include <mvLog.h>

#include "PipeMemoryWriteBlock.h"

#include <cassert>

// Defines
// ----------------------------------------------------------------------------------------

// Functions implementation
// ----------------------------------------------------------------------------------------
void PipeMemoryWriteBlock::Config(Configs * pConfigs, Utils * pUtils)
{
    mvLogLevelSet(MVLOG_WARN);

    // Sanity checks
    assert(pConfigs != nullptr);
    assert(pUtils != nullptr);
    assert(pUtils->pFrmPoolAlloc != nullptr);
    assert(pUtils->pRefKeeper != nullptr);

    // Copy configs and utils
    configs = *pConfigs;
    utils = *pUtils;
}

void PipeMemoryWriteBlock::Create()
{
    mvLogLevelSet(MVLOG_WARN);

    plgIn.Create(configs.pInStreamName, true);
    plgRefKeep.Create(PlgRefKeeper::Mode::KEEP_REF, utils.pRefKeeper);
    plgOut.Create(configs.pOutStreamName, configs.outSizeMax);
    plgRelease.Create(configs.pReleaseStreamName, false);
    plgRefRelease.Create(PlgRefKeeper::Mode::RELEASE_REF, utils.pRefKeeper);
}

void PipeMemoryWriteBlock::Start()
{
    mvLogLevelSet(MVLOG_WARN);

    plgIn.AddTo(&p);
    p.Add(&plgRefKeep);
    p.Add(&plgRefRelease);
    p.Add(&plgOut);
    plgRelease.AddTo(&p);

    plgIn.out.Link(&plgRefKeep.in);
    plgRefKeep.out.Link(&plgOut.in);
    plgRelease.out.Link(&plgRefRelease.in);

    p.Start();
}

void PipeMemoryWriteBlock::Stop()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Stop();
    p.Wait();
}

void PipeMemoryWriteBlock::Destroy()
{
    mvLogLevelSet(MVLOG_WARN);

    p.Delete();
}
