/*
 * VPUHostTimeSrv.hpp
 *
 *  Created on: Sep 2, 2020
 *      Author: apalfi
 */

#ifndef VPUHOSTTIMESRV_HOST_VPUHOSTTIMESRV_HPP_
#define VPUHOSTTIMESRV_HOST_VPUHOSTTIMESRV_HPP_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <string>

namespace vpual
{
namespace hosttimesrv
{
// Exported Types
// ----------------------------------------------------------------------------
/**
 * @brief Return status type for VPUHostTime Host functions
 */
enum class Ret
{
    SUCCESS = 0,
    ERR_NO_MEM,
    DEINIT_ERROR,
    DECODER_ERROR,
    XLINK_ERROR,
    OTHER_ERROR,
};

// Exported Functions
// ----------------------------------------------------------------------------
/**
 * @brief Start VPUHostTimeSrv on Host and creates decoder on MX.
 *
 * A new VPUAL Stub is created.
 *
 * XLink stream is opened for transferring the host time
 *
 * @retval SUCCESS If start was successful on both Host and MX.
 * FIXME
 */
Ret start();
Ret stop();

} // namespace hosttimesrv
} // namespace vpual

#endif /* VPUHOSTTIMESRV_HOST_VPUHOSTTIMESRV_HPP_ */
