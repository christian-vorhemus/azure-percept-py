/*
 * RmtPipeInferBlockRT_Common.h
 *
 *  Created on: Dec 13, 2020
 *      Author: apalfi
 */

#ifndef RMTPIPEINFERBLOCKRT_COMMON_H_
#define RMTPIPEINFERBLOCKRT_COMMON_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>

#include <RmtUtilsCache.h>
#include <PipeInferBlockRT.h>

namespace rmt {
namespace pipeinferblockrt {

// Defines
// ----------------------------------------------------------------------------
enum class CmdId : std::uint8_t
{
    Config,
};

typedef struct
{
    utils::CacheAligned<PipeInferBlockRTCfg> * pCfg;
} CmdConfig;

struct CmdMsg
{
    CmdId cmdId;
    PipeInferBlockRT * pRmt;
    union {
        CmdConfig config;
    } cmd;
};

} // namespace pipeinferblockrt
} // namespace rmt

#endif /* RMTPIPEINFERBLOCKRT_COMMON_H_ */
