/// ===========================================================================
///
///     @file:      VPUDeviceIo.cpp
///     @brief:     VPUDeviceIo host function implementations
///     @copyright: [INTEL CONFIDENTIAL] Copyright 2020 Intel Corporation.
/// This software and the related documents are Intel copyrighted materials,
/// and your use of them is governed by the express license under which they
/// were provided to you ("License"). Unless the License provides otherwise,
/// you may not use, modify, copy, publish, distribute, disclose or transmit
/// this software or the related documents without Intel's prior written
/// permission.
///
/// This software and the related documents are provided as is, with no express
/// or implied warranties, other than those that are expressly stated in the
/// License.
/// ===========================================================================

// Includes
// ----------------------------------------------------------------------------
#include "VPUDeviceIo.hpp"
#define MVLOG_UNIT_NAME VPUDeviceIo
#include <mvLog.h>
#include <VpualDispatcher.h>
#include <VpualMessage.h>
#include <XLink.h>
#include <secure_functions.h>
#include <cassert>

// Global Variables
// ----------------------------------------------------------------------------
static vpual::core::Stub *p_stub;
static bool initialized = false;
static streamId_t io_sid = INVALID_STREAM_ID;

namespace vpual
{
namespace devio
{
// Function Implementations
// ----------------------------------------------------------------------------

RetStatus_t init()
{
    mvLogLevelSet(MVLOG_WARN);
    mvLog(MVLOG_INFO, "Initializing VPUDeviceIo");

    p_stub = new core::Stub("DeviceIo");
    if (!p_stub)
    {
        mvLog(MVLOG_ERROR, "Could not allocate memory for Stub");
        return RetStatus_t::MEMORY_ERROR;
    }
    io_sid = XLinkOpenStream(0, VPU_DEVICE_IO_XLINK_CHN_NAME,
                             VPU_DEVICE_IO_XLINK_CHN_MAX_SIZE);
    if (io_sid == INVALID_STREAM_ID ||
        io_sid == INVALID_STREAM_ID_OUT_OF_MEMORY)
    {
        mvLog(MVLOG_ERROR, "Device I/O stream could not be opened!");
        return RetStatus_t::XLINK_ERROR;
    }

    action act = action::INIT;

    core::Message cmd;
    cmd.serialize(&act, sizeof(act));
    core::Message reply;
    p_stub->dispatch(cmd, reply);

    ActionCheck_t act_check;
    reply.deserialize(&act_check, sizeof(act_check));
    RetStatus_t ret;
    if (VALID_ACTION != act_check)
    {
        mvLog(MVLOG_ERROR, "Decoder error. Invalid action");
        XLinkError_t err = XLinkCloseStream(io_sid);
        if (err != X_LINK_SUCCESS)
        {
            mvLog(MVLOG_ERROR, "XLinkCloseStream error = %d", err);
        }
        return RetStatus_t::DECODER_ERROR;
    }
    else
    {
        ActionStatus_t status;
        reply.deserialize(&status, sizeof(status));
        switch (status)
        {
            case (ActionStatus_t::SUCCESS):
            {
                mvLog(MVLOG_INFO, "Initialized successfully");
                initialized = true;
                return RetStatus_t::SUCCESS;
                break;
            }
            case (ActionStatus_t::XLINK_ERROR):
            {
                mvLog(MVLOG_ERROR, "MX reported XLINK_ERROR");
                ret = RetStatus_t::XLINK_ERROR;
                break;
            }
            default:
            {
                mvLog(MVLOG_ERROR, "MX sent invalid status");
                ret = RetStatus_t::OTHER_ERROR;
                break;
            }
        }
    }

    XLinkError_t err = XLinkCloseStream(io_sid);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkCloseStream error = %d", err);
    }

    return ret;
}

RetStatus_t read(const std::string &devname, void *buffer, std::uint32_t count,
                 std::int32_t offset, std::uint32_t *read_bytes, int *ret_errno)
{
    if (!initialized)
    {
        mvLog(MVLOG_ERROR, "Component not initialized");
        return RetStatus_t::OTHER_ERROR;
    }
    assert(devname.size() != 0);
    assert(buffer != nullptr);
    assert(count > 0);
    mvLog(MVLOG_INFO,
          "Attempting to read %lu bytes from device %s starting at offset %ld",
          count, devname.c_str(), offset);

    action act = action::READ;
    core::Message cmd;
    cmd.serialize(&act, sizeof(act));
    std::uint32_t devname_size = static_cast<std::uint32_t>(devname.size());
    cmd.serialize(&devname_size, sizeof(devname_size));
    cmd.serialize(devname.data(), devname_size);
    cmd.serialize(&count, sizeof(count));
    cmd.serialize(&offset, sizeof(offset));
    core::Message reply;
    p_stub->dispatch(cmd, reply);

    ActionCheck_t act_check;
    reply.deserialize(&act_check, sizeof(act_check));
    if (VALID_ACTION != act_check)
    {
        mvLog(MVLOG_ERROR, "Decoder error. Invalid action");
        return RetStatus_t::DECODER_ERROR;
    }

    ActionStatus_t status;
    reply.deserialize(&status, sizeof(status));
    switch (status)
    {
        case (ActionStatus_t::SUCCESS):
        {
            reply.deserialize(read_bytes, sizeof(*read_bytes));
            if (*read_bytes != count)
            {
                mvLog(MVLOG_WARN, "Only %lu out of %lu bytes were read",
                      *read_bytes, count);
            }

            streamPacketDesc_t *packet;
            XLinkError_t err = XLinkReadData(io_sid, &packet);
            if (err != X_LINK_SUCCESS)
            {
                mvLog(MVLOG_ERROR, "XLinkReadData error = %d", err);
                return RetStatus_t::XLINK_ERROR;
            }
            if (packet->length != *read_bytes)
            {
                mvLog(MVLOG_ERROR, "Read buffer incomplete");
                err = XLinkReleaseData(io_sid);
                if (err != X_LINK_SUCCESS)
                {
                    mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
                }
                return RetStatus_t::XLINK_ERROR;
            }
            if (memcpy_s(buffer, count, packet->data, packet->length))
            {
                mvLog(MVLOG_ERROR, "memcpy_s error");
                return RetStatus_t::OTHER_ERROR;
            }
            err = XLinkReleaseData(io_sid);
            if (err != X_LINK_SUCCESS)
            {
                mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
            }
            mvLog(MVLOG_INFO, "Read operation successfully completed");
            return RetStatus_t::SUCCESS;
        }
        case (ActionStatus_t::DEVICE_OPEN_ERROR):
        {
            mvLog(MVLOG_ERROR, "Device could not be opened for reading");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::DEVICE_OPEN_ERROR;
        }
        case (ActionStatus_t::SEEK_ERROR):
        {
            mvLog(MVLOG_ERROR, "Could not reposition reading offset");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::SEEK_ERROR;
        }
        case (ActionStatus_t::READ_ERROR):
        {
            mvLog(MVLOG_ERROR, "Could not read from device");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::READ_ERROR;
        }
        case (ActionStatus_t::XLINK_ERROR):
        {
            mvLog(MVLOG_ERROR, "XLink error was encountered");

            return RetStatus_t::XLINK_ERROR;
        }
        case (ActionStatus_t::MEMORY_ERROR):
        {
            mvLog(MVLOG_ERROR, "MX has insufficient memory");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::MEMORY_ERROR;
        }
        default:
        {
            mvLog(MVLOG_ERROR, "MX returned undefined status");
            return RetStatus_t::OTHER_ERROR;
        }
    }
}

RetStatus_t write(const std::string &devname, void *buffer, std::uint32_t count,
                  std::int32_t offset, std::uint32_t *written_bytes,
                  int *ret_errno)
{
    if (!initialized)
    {
        mvLog(MVLOG_ERROR, "Component not initialized");
        return RetStatus_t::OTHER_ERROR;
    }
    assert(devname.size() != 0);
    assert(buffer != nullptr);
    assert(count > 0);
    mvLog(MVLOG_INFO,
          "Attempting to write %lu bytes to device %s starting at offset %ld",
          count, devname.c_str(), offset);

    action act = action::WRITE;
    core::Message cmd;
    cmd.serialize(&act, sizeof(act));
    std::uint32_t devname_size = static_cast<std::uint32_t>(devname.size());
    cmd.serialize(&devname_size, sizeof(devname_size));
    cmd.serialize(devname.data(), devname_size);
    cmd.serialize(&count, sizeof(count));
    cmd.serialize(&offset, sizeof(offset));

    XLinkError_t err = XLinkWriteData(io_sid, (const uint8_t *)buffer, count);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkWriteData error = %d", err);
        mvLog(MVLOG_ERROR, "Could not send data through XLink");
        return RetStatus_t::XLINK_ERROR;
    }

    core::Message reply;
    p_stub->dispatch(cmd, reply);
    ActionCheck_t act_check;
    reply.deserialize(&act_check, sizeof(act_check));
    if (VALID_ACTION != act_check)
    {
        mvLog(MVLOG_ERROR, "Decoder error. Invalid action");
        return RetStatus_t::DECODER_ERROR;
    }

    ActionStatus_t status;
    reply.deserialize(&status, sizeof(status));
    switch (status)
    {
        case (ActionStatus_t::SUCCESS):
        {
            reply.deserialize(written_bytes, sizeof(*written_bytes));
            if (*written_bytes != count)
            {
                mvLog(MVLOG_WARN, "Only %lu out of %lu bytes were written",
                      *written_bytes, count);
            }
            mvLog(MVLOG_INFO, "Write operation successfully completed");
            return RetStatus_t::SUCCESS;
        }
        case (ActionStatus_t::DEVICE_OPEN_ERROR):
        {
            mvLog(MVLOG_ERROR, "Device could not be opened for writing");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::DEVICE_OPEN_ERROR;
        }
        case (ActionStatus_t::SEEK_ERROR):
        {
            mvLog(MVLOG_ERROR, "Could not reposition writing offset");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::SEEK_ERROR;
        }
        case (ActionStatus_t::WRITE_ERROR):
        {
            mvLog(MVLOG_ERROR, "Could not write to device");
            std::int32_t errnum;
            reply.deserialize(&errnum, sizeof(errnum));
            mvLog(MVLOG_ERROR, "error: %s", strerror(errnum));
            if (ret_errno)
            {
                *ret_errno = static_cast<int>(errnum);
            }

            return RetStatus_t::WRITE_ERROR;
        }
        case (ActionStatus_t::XLINK_ERROR):
        {
            mvLog(MVLOG_ERROR, "XLink error was encountered");

            return RetStatus_t::XLINK_ERROR;
        }
        default:
        {
            mvLog(MVLOG_ERROR, "MX returned undefined status");
            return RetStatus_t::OTHER_ERROR;
        }
    }
}

RetStatus_t deinit()
{
    if (!initialized)
    {
        return RetStatus_t::SUCCESS;
    }
    mvLog(MVLOG_INFO, "Deinitializing VPUDeviceIo");
    action act = action::DEINIT;
    core::Message cmd;
    cmd.serialize(&act, sizeof(act));
    core::Message reply;
    p_stub->dispatch(cmd, reply);

    ActionCheck_t act_check;
    reply.deserialize(&act_check, sizeof(act_check));
    if (VALID_ACTION != act_check)
    {
        mvLog(MVLOG_ERROR, "Decoder error. Invalid action");
        return RetStatus_t::DECODER_ERROR;
    }

    ActionStatus_t status;
    reply.deserialize(&status, sizeof(status));
    switch (status)
    {
        case (ActionStatus_t::SUCCESS):
        {
            XLinkError_t err = XLinkCloseStream(io_sid);
            if (err != X_LINK_SUCCESS)
            {
                mvLog(MVLOG_ERROR, "XLinkCloseStream error = %d", err);
            }
            mvLog(MVLOG_INFO, "Deinitialized successfully");
            delete p_stub;
            initialized = false;
            return RetStatus_t::SUCCESS;
            break;
        }
        case (ActionStatus_t::XLINK_ERROR):
        {
            mvLog(MVLOG_ERROR, "MX could not close XLink stream");
            return RetStatus_t::XLINK_ERROR;
            break;
        }
        default:
        {
            mvLog(MVLOG_ERROR, "MX sent invalid status");
            return RetStatus_t::OTHER_ERROR;
            break;
        }
    }
}

} // namespace devio
} // namespace vpual
