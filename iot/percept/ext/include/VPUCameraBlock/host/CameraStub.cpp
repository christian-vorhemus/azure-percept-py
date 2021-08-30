#include <cassert> // for assert
#include <iostream> // for to_string

#include <PlgXlinkShared.h>
#include <secure_functions.h>
#include "xlink_utils.hpp"

#include "CameraStub.hpp"

#define MVLOG_UNIT_NAME CameraStub
#include <mvLog.h>

namespace vpual {
namespace stub {

Camera::Camera() : Stub("CameraBlock") { mvLogLevelSet(MVLOG_ERROR); }

void Camera::create()
{
    auto xLinkHandler = getXlinkDeviceHandler(0);
    std::string streamName;

    // Open XLink streams
    streamName = "CameraOutBgr" + std::to_string(Stub::id_);
    outBgrStream_.Open(xLinkHandler.linkId, streamName.c_str());
    streamName = "CameraOutVidEnc" + std::to_string(Stub::id_);
    outVidStream_.Open(xLinkHandler.linkId, streamName.c_str());
    streamName = "CameraBgrRelease" + std::to_string(Stub::id_);
    releaseBgrStream_.Open(xLinkHandler.linkId, streamName.c_str());

    // Notify device
    uint8_t data = (uint8_t)action::INIT;
    core::Message cmd;
    cmd.serialize(&data,sizeof(data));

    core::Message reply;
    dispatch(cmd,reply);

    mvLog(MVLOG_INFO,"H264 stream opened");
}

void Camera::start(CameraConfig *cameraConfig, EncoderConfig *encoderConfig) const {
    // Notify device
    std::uint8_t command = (std::uint8_t)action::START;
    core::Message cmd;
    cmd.serialize(&command, sizeof(action));
    cmd.serialize(&cameraConfig->mode, sizeof(cameraConfig->mode));
    cmd.serialize(&cameraConfig->fps, sizeof(cameraConfig->fps));
    cmd.serialize(&encoderConfig->enabled, sizeof(encoderConfig->enabled));
    cmd.serialize(&encoderConfig->bitrate, sizeof(encoderConfig->bitrate));
    cmd.serialize(&encoderConfig->framerate, sizeof(encoderConfig->framerate));
    cmd.serialize(&encoderConfig->gopSize, sizeof(encoderConfig->gopSize));
    core::Message rep;
    dispatch(cmd,rep);

    mvLog(MVLOG_INFO,"Camera started");
}

void Camera::pull_bgr(Frame * pFrame)
{
    mvLog(MVLOG_INFO,"Receiving BGR results");

    outBgrStream_.Read(pFrame);

    mvLog(MVLOG_INFO,"BGR payload received");
}

void Camera::pull_h264(Frame * pFrame)
{
    mvLog(MVLOG_INFO,"Receiving h264 results");

    outVidStream_.Read(pFrame);

    mvLog(MVLOG_INFO,"H264 packet received");
}

void Camera::release_bgr(const vpual::RmtMemHndl & rmtMemHndl)
{
    mvLog(MVLOG_INFO,"Releasing BGR results");

    Frame releaseFrame;
    releaseFrame.rmtMemHndl = rmtMemHndl;
    releaseBgrStream_.Write(releaseFrame);

    mvLog(MVLOG_INFO,"BGR release sent");
}

} // namespace stub
} // namespace vpual
