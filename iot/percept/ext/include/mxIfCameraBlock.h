/*
 * mxIfCameraBlock.h
 *
 *  Created on: Apr 6, 2020
 *      Author: apalfi
 */

#ifndef HOST_MXIF_MXIFCAMERABLOCK_H_
#define HOST_MXIF_MXIFCAMERABLOCK_H_

// Includes
// -------------------------------------------------------------------------------------
#include "mxIfMemoryHandle.h"

// Classes
// -------------------------------------------------------------------------------------
namespace mxIf
{
    class CameraBlock
    {
    public:
        enum class CamMode
        {
            CamMode_Native,
            CamMode_1080p,
            CamMode_720p
        };

        enum class Outputs
        {
            BGR,
            H264,
        };

        struct CameraConfig
        {
            CamMode mode;
            std::uint32_t fps;
        };

        struct EncoderConfig
        {
            bool enabled;
            std::uint32_t bitrate;
            std::uint16_t framerate;
            std::uint16_t gopSize;
        };

        CameraBlock(CamMode cam_mode);
        CameraBlock(const CameraConfig &cameraConfig, const EncoderConfig &encoderConfig);

        int EnableOutput(Outputs output, unsigned int flags = 0);
        int Start();
        MemoryHandle GetNextOutput(Outputs output);
        // TEMP
        void ReleaseOutput(Outputs output, MemoryHandle handle);

        CameraBlock(CameraBlock&&);
        CameraBlock& operator= (const CameraBlock&) = default;
        ~CameraBlock();
    private:
        struct CameraImpl;
        std::unique_ptr<CameraImpl> camera_;

        bool useDefaultCameraConfig;
        bool useDefaultEncoderConfig;
        CameraConfig cameraConfig;
        EncoderConfig encoderConfig;
    };
} // namespace mxIf

#endif /* HOST_MXIF_MXIFCAMERABLOCK_H_ */
