/*
 * RmtPipeCameraBlockRT_Decoder.h
 *
 *  Created on: Feb 14, 2021
 *      Author: apalfi
 */

#ifndef RMTPIPECAMERABLOCKRT_DECODER_H_
#define RMTPIPECAMERABLOCKRT_DECODER_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <RmtIFlicPipeWrap_Decoder.h>

#include <PipeCameraBlockRT.h>

namespace rmt {

// Defines
// ----------------------------------------------------------------------------
class PipeCameraBlockRTDecoder : public IFlicPipeWrapDecoder
{
public:
    static std::int32_t Init(PipeCameraBlockRT::Utils * pUtils);
    static PipeCameraBlockRT::Utils utils;

protected:
    static int32_t decoder(void * pParam);
};


} // namespace rmt

#endif /* RMTPIPECAMERABLOCKRT_DECODER_H_ */
