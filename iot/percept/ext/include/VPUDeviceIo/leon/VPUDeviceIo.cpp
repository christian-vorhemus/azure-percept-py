/// ===========================================================================
///
///     @file:      VPUDeviceIo.cpp
///     @brief:     VPUDeviceIo decoder implementation
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
#include <mvMacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>

// Defines
// ----------------------------------------------------------------------------
#define VPU_DEVICE_IO_L2ALIGN (64)

namespace vpual
{
namespace devio
{
// Function Implementations
// ----------------------------------------------------------------------------
VPUDeviceIo::VPUDeviceIo() : io_sid(INVALID_STREAM_ID)
{
    mvLogLevelSet(MVLOG_WARN);
}

void VPUDeviceIo::Decode(core::Message *cmd, core::Message *rep)
{
    action act;
    cmd->deserialize(&act, sizeof(act));

    switch (act)
    {
        case (action::INIT):
        {
            // Initialize VPUDeviceIo
            mvLog(MVLOG_DEBUG, "INIT command received");
            ActionCheck_t act_check = VALID_ACTION;
            rep->serialize(&act_check, sizeof(act_check));
            cmd_init(rep);
            mvLog(MVLOG_DEBUG, "INIT command completed");
            break;
        }
        case (action::READ):
        {
            // Read from device
            mvLog(MVLOG_DEBUG, "READ command received");
            ActionCheck_t act_check = VALID_ACTION;
            rep->serialize(&act_check, sizeof(act_check));
            cmd_read(cmd, rep);
            mvLog(MVLOG_DEBUG, "READ command completed");
            break;
        }
        case (action::WRITE):
        {
            // Write to device
            mvLog(MVLOG_DEBUG, "WRITE command received");
            ActionCheck_t act_check = VALID_ACTION;
            rep->serialize(&act_check, sizeof(act_check));
            cmd_write(cmd, rep);
            mvLog(MVLOG_DEBUG, "WRITE command completed");
            break;
        }
        case (action::DEINIT):
        {
            // Deinitialize VPUDeviceIo
            mvLog(MVLOG_DEBUG, "DEINIT command received");
            ActionCheck_t act_check = VALID_ACTION;
            rep->serialize(&act_check, sizeof(act_check));
            cmd_deinit(rep);
            mvLog(MVLOG_DEBUG, "DEINIT command completed");
            break;
        }
        default:
        {
            ActionCheck_t act_check = INVALID_ACTION;
            rep->serialize(&act_check, sizeof(act_check));
            mvLog(MVLOG_ERROR, "Received invalid action request");
            break;
        }
    }
}

void VPUDeviceIo::cmd_init(core::Message *rep)
{
    mvLog(MVLOG_INFO, "Initializing VPUDeviceIo");
    io_sid = XLinkOpenStream(0, VPU_DEVICE_IO_XLINK_CHN_NAME,
                             VPU_DEVICE_IO_XLINK_CHN_MAX_SIZE);
    if (io_sid == INVALID_STREAM_ID ||
        io_sid == INVALID_STREAM_ID_OUT_OF_MEMORY)
    {
        mvLog(MVLOG_ERROR, "Could not open I/O buffer XLink Stream");
        ActionStatus_t status = ActionStatus_t::XLINK_ERROR;
        rep->serialize(&status, sizeof(status));
    }
    else
    {
        mvLog(MVLOG_INFO, "Successfully initialized VPUDeviceIo");
        ActionStatus_t status = ActionStatus_t::SUCCESS;
        rep->serialize(&status, sizeof(status));
    }
}

void VPUDeviceIo::cmd_read(core::Message *cmd, core::Message *rep)
{
    std::uint32_t devname_size;
    cmd->deserialize(&devname_size, sizeof(devname_size));
    std::string devname(static_cast<std::size_t>(devname_size), 0);
    cmd->deserialize(&devname[0], devname_size);

    std::uint32_t count;
    cmd->deserialize(&count, sizeof(count));

    std::int32_t offset;
    cmd->deserialize(&offset, sizeof(offset));

    mvLog(MVLOG_INFO,
          "Attempting to read %lu bytes from device %s at offset %ld", count,
          devname.c_str(), offset);

    int fd = open(devname.c_str(), O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        mvLog(MVLOG_ERROR, "Could not open device %s for reading",
              devname.c_str());
        ActionStatus_t status = ActionStatus_t::DEVICE_OPEN_ERROR;
        rep->serialize(&status, sizeof(status));

        // Return the errno to the Host
        std::int32_t errnum = static_cast<std::int32_t>(errno);
        rep->serialize(&errnum, sizeof(errnum));
        return;
    }

    mvLog(MVLOG_INFO, "Successfully opened device %s", devname.c_str());

    if (offset != 0)
    {
        off_t ret = lseek(fd, (off_t)offset, SEEK_SET);
        if (ret == (off_t)(-1))
        {
            mvLog(MVLOG_ERROR, "Failed to reposition read offset to %ld",
                  offset);
            ActionStatus_t status = ActionStatus_t::SEEK_ERROR;
            rep->serialize(&status, sizeof(status));

            // Return the errno to the Host
            std::int32_t errnum = static_cast<std::int32_t>(errno);
            rep->serialize(&errnum, sizeof(errnum));
            close(fd);
            return;
        }
        else
        {
            mvLog(MVLOG_INFO, "Successfully repositioned read offset to %ld",
                  offset);
        }
    }

    char *buff = reinterpret_cast<char *>(aligned_alloc(
        VPU_DEVICE_IO_L2ALIGN, ALIGN_UP(count, VPU_DEVICE_IO_L2ALIGN)));
    if (!buff)
    {
        mvLog(MVLOG_ERROR, "Could not allocate buffer to read into");
        ActionStatus_t status = ActionStatus_t::MEMORY_ERROR;
        rep->serialize(&status, sizeof(status));

        std::int32_t errnum = ENOMEM;
        rep->serialize(&errnum, sizeof(errnum));
        close(fd);
        return;
    }

    ssize_t read_bytes = read(fd, (void *)buff, (size_t)count);
    if (read_bytes == -1)
    {
        mvLog(MVLOG_ERROR, "Could not read from device %s", devname.c_str());
        ActionStatus_t status = ActionStatus_t::READ_ERROR;
        rep->serialize(&status, sizeof(status));

        std::int32_t errnum = static_cast<std::int32_t>(errno);
        rep->serialize(&errnum, sizeof(errnum));
        free(buff);
        close(fd);
        return;
    }
    close(fd);
    if ((std::uint32_t)read_bytes != count)
    {
        mvLog(MVLOG_WARN, "Only %ld out of %ld bytes were read", read_bytes,
              count);
    }

    XLinkError_t err =
        XLinkWriteData(io_sid, (const uint8_t *)buff, read_bytes);
    free(buff);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkWriteData error = %d", err);
        mvLog(MVLOG_ERROR, "Could not send read data through XLink");
        ActionStatus_t status = ActionStatus_t::XLINK_ERROR;
        rep->serialize(&status, sizeof(status));
        return;
    }

    ActionStatus_t status = ActionStatus_t::SUCCESS;
    rep->serialize(&status, sizeof(status));

    std::uint32_t read_bytes_u32 = static_cast<std::uint32_t>(read_bytes);
    rep->serialize(&read_bytes_u32, sizeof(read_bytes_u32));

    mvLog(MVLOG_INFO, "Successfully read data");
}

void VPUDeviceIo::cmd_write(core::Message *cmd, core::Message *rep)
{
    std::uint32_t devname_size;
    cmd->deserialize(&devname_size, sizeof(devname_size));
    std::string devname(static_cast<std::size_t>(devname_size), 0);
    cmd->deserialize(&devname[0], devname_size);

    std::uint32_t count;
    cmd->deserialize(&count, sizeof(count));

    std::int32_t offset;
    cmd->deserialize(&offset, sizeof(offset));

    mvLog(MVLOG_INFO,
          "Attempting to write %lu bytes to device %s at offset %ld", count,
          devname.c_str(), offset);

    streamPacketDesc_t *packet;
    XLinkError_t err = XLinkReadData(io_sid, &packet);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkReadData error = %d", err);

        ActionStatus_t status = ActionStatus_t::XLINK_ERROR;
        rep->serialize(&status, sizeof(status));
        return;
    }
    if (packet->length != count)
    {
        mvLog(MVLOG_ERROR, "Received incomplete data buffer");

        ActionStatus_t status = ActionStatus_t::XLINK_ERROR;
        rep->serialize(&status, sizeof(status));
        return;
    }

    int fd = open(devname.c_str(), O_WRONLY, S_IWUSR | S_IWGRP | S_IWOTH);
    if (fd < 0)
    {
        mvLog(MVLOG_ERROR, "Could not open device %s for writing",
              devname.c_str());
        ActionStatus_t status = ActionStatus_t::DEVICE_OPEN_ERROR;
        rep->serialize(&status, sizeof(status));

        // Return the errno to the Host
        std::int32_t errnum = static_cast<std::int32_t>(errno);
        rep->serialize(&errnum, sizeof(errnum));
        err = XLinkReleaseData(io_sid);
        if (err != X_LINK_SUCCESS)
        {
            mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
        }
        return;
    }
    mvLog(MVLOG_INFO, "Successfully opened device %s", devname.c_str());

    if (offset != 0)
    {
        off_t ret = lseek(fd, (off_t)offset, SEEK_SET);
        if (ret == (off_t)(-1))
        {
            mvLog(MVLOG_ERROR, "Failed to reposition write offset to %ld",
                  offset);
            ActionStatus_t status = ActionStatus_t::SEEK_ERROR;
            rep->serialize(&status, sizeof(status));

            // Return the errno to the Host
            std::int32_t errnum = static_cast<std::int32_t>(errno);
            rep->serialize(&errnum, sizeof(errnum));
            close(fd);
            err = XLinkReleaseData(io_sid);
            if (err != X_LINK_SUCCESS)
            {
                mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
            }
            return;
        }
        else
        {
            mvLog(MVLOG_INFO, "Successfully repositioned write offset to %ld",
                  offset);
        }
    }
    ssize_t written_bytes = write(fd, (void *)packet->data, (size_t)count);
    if (written_bytes == -1)
    {
        mvLog(MVLOG_ERROR, "Could not write to device %s", devname.c_str());
        ActionStatus_t status = ActionStatus_t::WRITE_ERROR;
        rep->serialize(&status, sizeof(status));

        std::int32_t errnum = static_cast<std::int32_t>(errno);
        rep->serialize(&errnum, sizeof(errnum));
        close(fd);
        err = XLinkReleaseData(io_sid);
        if (err != X_LINK_SUCCESS)
        {
            mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
        }
        return;
    }
    close(fd);
    err = XLinkReleaseData(io_sid);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkReleaseData error %d", err);
    }
    if ((std::uint32_t)written_bytes != count)
    {
        mvLog(MVLOG_WARN, "Only %ld out of %lu were written", written_bytes,
              count);
    }
    ActionStatus_t status = ActionStatus_t::SUCCESS;
    rep->serialize(&status, sizeof(status));

    std::uint32_t written_bytes_u32 = static_cast<std::uint32_t>(written_bytes);
    rep->serialize(&written_bytes, sizeof(written_bytes));

    mvLog(MVLOG_INFO, "Write operation completed successfully");
}

void VPUDeviceIo::cmd_deinit(core::Message *rep)
{
    mvLog(MVLOG_INFO, "Deinitializing VPUDeviceIo");

    XLinkError_t err = XLinkCloseStream(io_sid);
    if (err != X_LINK_SUCCESS)
    {
        mvLog(MVLOG_ERROR, "XLinkCloseStream error = %d", err);
        ActionStatus_t status = ActionStatus_t::XLINK_ERROR;
        rep->serialize(&status, sizeof(status));
        return;
    }

    mvLog(MVLOG_INFO, "Successfully deinitialized VPUDeviceIo");
    ActionStatus_t status = ActionStatus_t::SUCCESS;
    rep->serialize(&status, sizeof(status));
}

} // namespace devio
} // namespace vpual
