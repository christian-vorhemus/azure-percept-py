/*
 * VPUHostTimeSrvDefs.hpp
 *
 *  Created on: Sep 2, 2020
 *      Author: apalfi
 */

#ifndef VPUHOSTTIMESRV_SHARED_VPUHOSTTIMESRVDEFS_HPP_
#define VPUHOSTTIMESRV_SHARED_VPUHOSTTIMESRVDEFS_HPP_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>

// Defines
// ----------------------------------------------------------------------------
namespace vpual
{
namespace hosttimesrv
{
// Exported Types
// ----------------------------------------------------------------------------
/**
 * @brief Commands for the decoder
 */
enum class Command : std::uint8_t
{
    PROPOSE = 0,
    UPDATE,
};

/**
 * @brief Type used to express if Host sent a valid action.
 */
enum class DecoderRet : std::uint8_t
{
    SUCCESS,
    INVALID_CMD,
};

} // namespace devio
} // namespace hosttimesrv

#endif /* VPUHOSTTIMESRV_SHARED_VPUHOSTTIMESRVDEFS_HPP_ */
