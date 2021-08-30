/*
 * mxIf.h
 *
 *  Created on: Mar 11, 2020
 *      Author: apalfi
 */

#ifndef HOST_MXIF_MXIF_H_
#define HOST_MXIF_MXIF_H_

// Includes
// -------------------------------------------------------------------------------------
#include <string>
#include <stdint.h>

#include <mxIfInferBlock.h>
#include <mxIfCameraBlock.h>
#include <mxIfMemoryHandle.h>

// Defines
// -------------------------------------------------------------------------------------
namespace mxIf
{
    struct FrameResult
    {
        void* pBuf;
        uint32_t bufSize;
        int64_t seqNo;
        int64_t ts;
    };

    void Boot(const std::string& mvcmdPath);
    void Reset();

    CameraBlock CreateCameraBlock(CameraBlock::CamMode cam_mode = CameraBlock::CamMode::CamMode_Native);
    CameraBlock CreateCameraBlock(const CameraBlock::CameraConfig &cameraConfig,
                                  const CameraBlock::EncoderConfig &encoderConfig);
    // todo: refactor this
    InferBlock CreateInferBlock(const std::string& blobfile);
} // namespace mxIf


#endif /* HOST_MXIF_MXIF_H_ */
