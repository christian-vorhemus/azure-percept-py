#pragma once

#include <vector>

#include <XLink.h>
#include <VpualDispatcher.h>
#include <VPUBlockTypes.h>
#include <VPUBlockXLink.h>

namespace vpual {
namespace stub {

class Camera : public core::Stub {
    // Supported Stub functionality
    enum class action : char {
        INIT = 0,
        // ToDo: improve serialization
        START,
    };
public:
    struct CameraConfig;
    struct EncoderConfig;

    Camera();

    void create();
    void start(CameraConfig *cameraConfig, EncoderConfig *encoderConfig) const;

    struct CameraConfig
    {
        std::uint8_t mode;
        std::uint32_t fps;
    };

    struct EncoderConfig
    {
        std::uint8_t enabled;
        std::uint32_t bitrate;
        std::uint16_t framerate;
        std::uint16_t gopSize;
    };

    void pull_bgr(Frame * pFrame);
    void pull_h264(Frame * pFrame);
    void release_bgr(const RmtMemHndl & rmtMemHndl);

private:
    XLinkVpuOut outBgrStream_;
    XLinkVpuOut outVidStream_;
    XLinkVpuIn releaseBgrStream_;
};

} // namespace stub
} // namespace vpual

