/*
 * RmtPipeInferBlockRT_Stub.h
 *
 *  Created on: Dec 13, 2020
 *      Author: apalfi
 */

#ifndef RMTPIPEINFERBLOCKRT_STUB_H_
#define RMTPIPEINFERBLOCKRT_STUB_H_

// Includes
// ----------------------------------------------------------------------------
#include <RmtUtilsCache.h>

#include <RmtIFlicPipeWrap_Stub.h>
#include <PipeInferBlockRT.h>

namespace rmt {

// Defines
// ----------------------------------------------------------------------------
class PipeInferBlockRTStub : public IFlicPipeWrapStub {
public:
    PipeInferBlockRTStub();
    void Bind(PipeInferBlockRT * pRmt);

    void Config(rmt::utils::CacheAligned<PipeInferBlockRTCfg> * pCfg);

protected:
    PipeInferBlockRT * pRmt;
};

} // namespace rmt

#endif /* RMTPIPEINFERBLOCKRT_STUB_H_ */
