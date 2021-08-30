/*
 * XLinkWrap_Host.cpp
 *
 *  Created on: Jul 20, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------------------
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "XLinkWrap_Host.hpp"

#define MVLOG_UNIT_NAME XLinkWrap
#include <mvLog.h>

namespace xlinkwrap {
namespace host {

// Local data
// ----------------------------------------------------------------------------------------
static XLinkGlobalHandler_t xLinkGlobalHandle = {
    .profEnable = 0,
    .profilingData = {0.0, 0.0, 0, 0, 0, 0},
    .loglevel = 0,
    .protocol = USB_VSC
};

// Functions implementation
// ----------------------------------------------------------------------------------------
int32_t Device::boot(const std::string& mvcmd_path)
{
    mvLogLevelSet(MVLOG_ERROR);

    int32_t ret = 0;
    XLinkError_t xlink_ret = X_LINK_SUCCESS;
    deviceDesc_t xLinkInDeviceDesc = { X_LINK_USB_VSC, X_LINK_ANY_PLATFORM };

    // Initialize the XLink component
    // ToDo: only initialize once
    xlink_ret = XLinkInitialize(&xLinkGlobalHandle);
    if (xlink_ret != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR, "XLinkInitialize failed; xlink_ret=%d", xlink_ret);
        // ToDo: convert from XLink to errno
        ret = EIO;
        goto exit;
    }
    mvLog(MVLOG_INFO, "Initialized XLink component");

    if (!mvcmd_path.empty()) {
        // Find and boot MX
        // ToDo: add timeout
        xlink_ret = XLinkFindFirstSuitableDevice(X_LINK_UNBOOTED, xLinkInDeviceDesc, &deviceDesc_);
        if (xlink_ret == X_LINK_SUCCESS) {
            mvLog(MVLOG_INFO, "Found unbooted MX device name %s", deviceDesc_.name);
            xlink_ret = XLinkBoot(&deviceDesc_, mvcmd_path.c_str());
            if(xlink_ret != X_LINK_SUCCESS) {
                mvLog(MVLOG_ERROR, "Failed to boot the MX device: %s :: %s, err code %d \n", deviceDesc_.name, mvcmd_path.c_str(), xlink_ret);
                // ToDo: convert from XLink to errno
                ret = EIO;
                goto exit;
            }
            mvLog(MVLOG_INFO, "MX device booted");
        } else {
            mvLog(MVLOG_WARN, "MX device might be already booted");
        }
    }

    // Try to find booted myriad
    // ToDo: add timeout
    xlink_ret = X_LINK_COMMUNICATION_UNKNOWN_ERROR;
    while (xlink_ret != X_LINK_SUCCESS) {
        xlink_ret = XLinkFindFirstSuitableDevice(X_LINK_BOOTED, xLinkInDeviceDesc, &deviceDesc_);
        if (xlink_ret != X_LINK_SUCCESS) {
            mvLog(MVLOG_WARN, "Failed to find MX booted device. Retrying...");
            usleep(100000);
        }
    }
    mvLog(MVLOG_INFO,"Found booted MX with device name %s\n", deviceDesc_.name);

    // Prepare device handle
    deviceHandle_.devicePath = deviceDesc_.name;
    deviceHandle_.protocol = deviceDesc_.protocol;

    // Try to connect to myriad
    // ToDo: add timeout
    xlink_ret = X_LINK_COMMUNICATION_UNKNOWN_ERROR;
    while (xlink_ret != X_LINK_SUCCESS) {
        xlink_ret = XLinkConnect(&deviceHandle_);
    }

    mvLog(MVLOG_INFO,"Successfully connected to Myriad device!");

exit:
    mvLog(MVLOG_DEBUG, "Exit with %d (%s)", ret, strerror(ret));
    return ret;
}

int32_t Device::reset()
{
    int32_t ret = 0;
    XLinkError_t xlink_ret = X_LINK_SUCCESS;

    xlink_ret = XLinkResetRemote(deviceHandle_.linkId);
    if (xlink_ret != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR,"disconnecting XLink. status = %d", xlink_ret);
        // ToDo: convert from XLink to errno
        ret = EIO;
        goto exit;
    }
    else {
        mvLog(MVLOG_INFO,"Device reset successfully");
    }

exit:
    mvLog(MVLOG_DEBUG, "Exit with %d (%s)", ret, strerror(ret));
    return ret;
}

} // namespace host
} // namespace xlinkwrap
