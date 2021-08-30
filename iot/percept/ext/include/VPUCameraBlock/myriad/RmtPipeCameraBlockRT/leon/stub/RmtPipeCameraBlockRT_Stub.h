/*
 * RmtPipeCameraBlockRT_Stub.h
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

#ifndef RMTPIPECAMERABLOCKRT_STUB_H_
#define RMTPIPECAMERABLOCKRT_STUB_H_

// Includes
// ----------------------------------------------------------------------------
#include <RmtUtilsCache.h>

#include <RmtIFlicPipeWrap_Stub.h>
#include <PipeCameraBlockRT.h>

namespace rmt {

// Defines
// ----------------------------------------------------------------------------
class PipeCameraBlockRTStub : public IFlicPipeWrapStub {
public:
    PipeCameraBlockRTStub();
    void Bind(PipeCameraBlockRT * pRmt);

    void Config(rmt::utils::CacheAligned<PipeCameraBlockRT::Configs> * pConfigs);

protected:
    PipeCameraBlockRT * pRmt;
};

} // namespace rmt

#endif /* RMTPIPECAMERABLOCKRT_STUB_H_ */
