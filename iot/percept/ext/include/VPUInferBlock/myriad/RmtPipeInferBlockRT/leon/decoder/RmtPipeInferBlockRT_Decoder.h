/*
 * RmtPipeInferBlockRT_Decoder.h
 *
 *  Created on: Dec 13, 2020
 *      Author: apalfi
 */

#ifndef RMTPIPEINFERBLOCKRT_DECODER_H_
#define RMTPIPEINFERBLOCKRT_DECODER_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <RmtIFlicPipeWrap_Decoder.h>

#include <PipeInferBlockRT.h>

namespace rmt {

// Defines
// ----------------------------------------------------------------------------
class PipeInferBlockRTDecoder : public IFlicPipeWrapDecoder
{
public:
    static std::int32_t Init(PipeInferBlockRTUtils * pUtils);
    static PipeInferBlockRTUtils utils;

protected:
    static int32_t decoder(void *pParam);
};


} // namespace rmt

#endif /* RMTPIPEINFERBLOCKRT_DECODER_H_ */
