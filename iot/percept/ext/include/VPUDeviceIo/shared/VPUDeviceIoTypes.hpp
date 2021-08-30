//  VPUDeviceIoTypes.hpp -*- C++ -*-
/// ===========================================================================
///
///     @file:      VPUDeviceIoTypes.hpp
///     @brief:     VPUDeviceIo common types used on host and MX
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
#ifndef _SHARED_VPU_DEVICE_IO_TYPES_HPP_
#define _SHARED_VPU_DEVICE_IO_TYPES_HPP_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>

// Defines
// ----------------------------------------------------------------------------
#define VPU_DEVICE_IO_XLINK_CHN_MAX_SIZE (1 * 1024 * 1024)
#define VPU_DEVICE_IO_XLINK_CHN_NAME "DeviceIo"

namespace vpual
{
namespace devio
{
// Exported Types
// ----------------------------------------------------------------------------
/**
 * @brief Type used by both Host and MX for executing commands.
 */
enum class action : std::uint8_t
{
    INIT,
    READ,
    WRITE,
    DEINIT,
};

/**
 * @brief Type used by MX to tell Host if commands were executed successfully.
 */
enum class ActionStatus_t : std::uint8_t
{
    SUCCESS = 0,
    DEVICE_OPEN_ERROR,
    SEEK_ERROR,
    READ_ERROR,
    WRITE_ERROR,
    MEMORY_ERROR,
    XLINK_ERROR,
};

/**
 * @brief Type used to express if Host sent a valid action.
 */
enum ActionCheck_t : std::uint8_t
{
    VALID_ACTION,
    INVALID_ACTION,
};

} // namespace devio
} // namespace vpual

#endif // _SHARED_VPU_DEVICE_IO_TYPES_HPP_
