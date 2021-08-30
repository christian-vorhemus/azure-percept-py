/*
 * RmtPipeCameraBlockRT_Common.h
 *
 *  Created on: Feb 12, 2021
 *      Author: apalfi
 */

#ifndef RMTPIPECAMERABLOCKRT_COMMON_H_
#define RMTPIPECAMERABLOCKRT_COMMON_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>

#include <RmtUtilsCache.h>
#include <PipeCameraBlockRT.h>

namespace rmt {
namespace pipecamerablockrt {

// Defines
// ----------------------------------------------------------------------------
enum class CmdId : std::uint8_t
{
    Config,
};

typedef struct
{
    utils::CacheAligned<PipeCameraBlockRT::Configs> * pConfigs;
} CmdConfig;

struct CmdMsg
{
    CmdId cmdId;
    PipeCameraBlockRT * pRmt;
    union {
        CmdConfig config;
    } cmd;
};

} // namespace pipecamerablockrt
} // namespace rmt

#endif /* RMTPIPECAMERABLOCKRT_COMMON_H_ */
