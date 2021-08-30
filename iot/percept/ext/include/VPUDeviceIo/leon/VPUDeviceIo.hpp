//  VPUDeviceIo.hpp -*- C++ -*-
/// ===========================================================================
///
///     @file:      VPUDeviceIo.hpp
///     @brief:     VPUDeviceIo decoder prototype
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
#ifndef _LEON_VPU_DEVICE_IO_HPP_
#define _LEON_VPU_DEVICE_IO_HPP_

// Includes
// ----------------------------------------------------------------------------
#include "VPUDeviceIoTypes.hpp"
#include "Decoder.h"
#include <XLink.h>

namespace vpual
{
namespace devio
{
// Exported Classes
// ----------------------------------------------------------------------------
/**
 * @brief VPUDeviceIo decoder.
 */
class VPUDeviceIo final : public core::Decoder
{
public:
    VPUDeviceIo();

    void Decode(core::Message *cmd, core::Message *rep);

    ~VPUDeviceIo() = default;

private:
    /**
     * @brief XLink channel for transfering read/write buffers.
     */
    streamId_t io_sid;

    /**
     * @brief Initialize VPUDeviceIo.
     *
     * Opens the XLink stream for read/write buffers.
     *
     * @param[out] rep VPUAL Message sent to Host as reply, containing
     * ActionStatus_t::SUCCESS if initialization was successful or
     * ActionStatus_t::XLINK_ERROR if the XLink stream could not be opened.
     */
    void cmd_init(core::Message *rep);

    /**
     * @brief Read data from device.
     *
     * Read data is transfered through the I/O XLink stream.
     *
     * @param[in] cmd VPUAL Message coming from Host, expected to contain
     * device name size, device name, number of bytes to be read and offset.
     * @param[out] rep VPUAL Message sent to Host as reply, containing
     * ActionStatus_t::SUCCESS together with the number of bytes read in case
     * everything was successful,
     *
     * ActionStatus_t::DEVICE_OPEN_ERROR together with the corresponding errno
     * in case of error when trying to open the device,
     *
     * ActionStatus_t::SEEK_ERROR together with the corresponding errno in case
     * the offset was non-zero and there was an error when trying to reposition
     * the read offset,
     *
     * ActionStatus_t::MEMORY_ERROR together with the corresponding errno in case
     * there was an error when trying to allocate the buffer in which to read,
     *
     * ActionStatus_t::READ_ERROR together with the corresponding errno in case
     * there was an error when trying to read from the device,
     *
     * ActionStatus_t::XLINK_ERROR if there was an error when trying to transfer
     * the read data to Host.
     */
    void cmd_read(core::Message *cmd, core::Message *rep);

    /**
     * @brief Write data to device.
     *
     * Data to be written is transfered by the Host through the I/O XLink stream
     *
     * @param[in] cmd VPUAL Message coming from Host, expected to contain
     * device name size, device name, number of bytes to be written and offset.
     * @param[out] rep VPUAL Message sent to Host as reply, containing
     * ActionStatus_t::SUCCESS together with the number of bytes written in case
     * everything was successful,
     *
     * ActionStatus_t::DEVICE_OPEN_ERROR together with the corresponding errno
     * in case of error when trying to open the device,
     *
     * ActionStatus_t::SEEK_ERROR together with the corresponding errno in case
     * the offset was non-zero and there was an error when trying to reposition
     * the write offset,
     *
     * ActionStatus_t::WRITE_ERROR together with the corresponding errno in case
     * there was an error when trying to write to the device,
     *
     * ActionStatus_t::XLINK_ERROR if there was an error when trying to read
     * the data transfered by Host through XLink.
     */
    void cmd_write(core::Message *cmd, core::Message *rep);

    /**
     * @brief Initialize VPUDeviceIo.
     *
     * Closes the XLink stream for read/write buffers.
     *
     * @param[out] rep VPUAL Message sent to Host as reply, containing
     * ActionStatus_t::SUCCESS if initialization was successful or
     * ActionStatus_t::XLINK_ERROR if the XLink stream could not be closed.
     */
    void cmd_deinit(core::Message *rep);
};

} // namespace devio
} // namespace vpual

#endif // _LEON_VPU_DEVICE_IO_HPP_
