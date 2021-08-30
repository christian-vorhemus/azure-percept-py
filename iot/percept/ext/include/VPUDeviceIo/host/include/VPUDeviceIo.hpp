//  VPUDeviceIo.hpp -*- C++ -*-
/// ===========================================================================
///
///     @file:      VPUDeviceIo.hpp
///     @brief:     VPUDeviceIo host types and function prototypes
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
#ifndef _HOST_VPU_DEVICE_IO_HPP_
#define _HOST_VPU_DEVICE_IO_HPP_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include "VPUDeviceIoTypes.hpp"

namespace vpual
{
namespace devio
{
// Exported Types
// ----------------------------------------------------------------------------
/**
 * @brief Return status type for VPUDeviceIo Host functions
 */
enum class RetStatus_t
{
    SUCCESS,
    DECODER_ERROR,
    DEVICE_OPEN_ERROR,
    SEEK_ERROR,
    READ_ERROR,
    WRITE_ERROR,
    XLINK_ERROR,
    MEMORY_ERROR,
    OTHER_ERROR,
};

// Exported Functions
// ----------------------------------------------------------------------------
/**
 * @brief Initialize VPUDeviceIo on both Host and MX.
 *
 * A new Stub is created.
 *
 * I/O XLink stream is opened for transfering read/write buffers.
 *
 * @retval SUCCESS If initialization was successful on both Host and MX.
 * @retval DECODER_ERROR If action was not recognized by the decoder.
 * @retval XLINK_ERROR If XLink stream could not be opened.
 * @retval MEMORY_ERROR If there is not enough memory for creating a Stub instance.
 * @retval OTHER_ERROR If MX sent an invalid status.
 */
RetStatus_t init();

/**
 * @brief Read data from device.
 *
 * MX will transfer the data through the I/O XLink stream.
 *
 * @param[in] devname Name of device to read from.
 * @param[out] buffer Buffer in which read data will be placed.
 * @param[in] count Number of bytes to read.
 * @param[in] offset Byte number to start the reading at.
 * @param[out] read_bytes Number of bytes which could be read from device.
 * If read_bytes is different from count, it is not necessarily an error.
 * @param[out] ret_errno Corresponding errno value, set accordingly when any
 * of the following errors are returned: DEVICE_OPEN_ERROR, SEEK_ERROR,
 * READ_ERROR, MEMORY_ERROR. Optional parameter.
 *
 * @retval SUCCESS If data was successfully read from the device.
 * Returned even if actual number of read bytes is different from count.
 * @retval DECODER_ERROR If action was not recognized by the decoder.
 * @retval DEVICE_OPEN_ERROR If there was an error when opening the device.
 * @retval SEEK_ERROR If offset is non-zero and there was an error when trying
 * to reposition the device's read offset.
 * @retval READ_ERROR If there was an error when trying to read from device.
 * @retval MEMORY_ERROR If MX could not allocate memory for reading.
 * @retval XLINK_ERROR If there was a problem with the XLink transfer.
 * @retval OTHER_ERROR For other miscellaneous errors.
 */
RetStatus_t read(const std::string &devname, void *buffer, std::uint32_t count,
                 std::int32_t offset, std::uint32_t *read_bytes,
                 int *ret_errno = nullptr);

/**
 * @brief Write data to device.
 *
 * Host will transfer the data through the I/O XLink stream.
 *
 * @param[in] devname Name of device to write to.
 * @param[out] buffer Buffer containing the data to be written.
 * @param[in] count Number of bytes to write.
 * @param[in] offset Byte number to start the writing at.
 * @param[out] written_bytes Number of bytes which could be written to the
 * device. If written_bytes is different from count, it is not necessarily an
 * error.
 * @param[out] ret_errno Corresponding errno value, set accordingly when any
 * of the following errors are returned: DEVICE_OPEN_ERROR, SEEK_ERROR,
 * WRITE_ERROR. Optional parameter.
 *
 * @retval SUCCESS If data was successfully written to the device.
 * Returned even if actual number of written bytes is different from count.
 * @retval DECODER_ERROR If action was not recognized by the decoder.
 * @retval DEVICE_OPEN_ERROR If there was an error when opening the device.
 * @retval SEEK_ERROR If offset is non-zero and there was an error when trying
 * to reposition the device's write offset.
 * @retval WRITE_ERROR If there was an error when trying to write to the device.
 * @retval XLINK_ERROR If there was a problem with the XLink transfer.
 * @retval OTHER_ERROR For other miscellaneous errors.
 */
RetStatus_t write(const std::string &devname, void *buffer, std::uint32_t count,
                  std::int32_t offset, std::uint32_t *written_bytes,
                  int *ret_errno = nullptr);

/**
 * @brief Deinitialize VPUDeviceIo on both Host and MX.
 *
 * The I/O XLink channel is closed and the VPUDeviceIo Stub is deleted.
 *
 * @retval SUCCESS If initialization was successful on both Host and MX.
 * @retval DECODER_ERROR If action was not recognized by the decoder.
 * @retval XLINK_ERROR If MX could not close the XLink stream.
 * @retval OTHER_ERROR If MX sent an invalid status.
 */
RetStatus_t deinit();

} // namespace devio
} // namespace vpual

#endif // _HOST_VPU_DEVICE_IO_HPP_
