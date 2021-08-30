/*
 * VPUHostTimeSrv.cpp
 *
 *  Created on: Sep 2, 2020
 *      Author: apalfi
 */

// Includes
// ----------------------------------------------------------------------------
#include <time.h>
#include <rtems.h>
#include <mvTimestamp.h>

#include <TimeSync.h>

#include "VPUHostTimeSrvDefs.hpp"
#include "VPUHostTimeSrv.hpp"

#define MVLOG_UNIT_NAME VPUHostTimeSrv
#include <mvLog.h>

// Defines
// ----------------------------------------------------------------------------

namespace vpual
{
namespace hosttimesrv
{
// Function Implementations
// ----------------------------------------------------------------------------
VPUHostTimeSrv::VPUHostTimeSrv() { mvLogLevelSet(MVLOG_WARN); }

void VPUHostTimeSrv::Decode(core::Message *p_cmd_msg, core::Message *p_resp_msg)
{
    Command cmd;
    p_cmd_msg->deserialize(&cmd, sizeof(cmd));

    switch (cmd)
    {
        case (Command::PROPOSE):
        {
            mvLog(MVLOG_DEBUG, "PROPOSE command received");
            DecoderRet decoder_ret = DecoderRet::SUCCESS;
            p_resp_msg->serialize(&decoder_ret, sizeof(DecoderRet));
            propose(p_cmd_msg, p_resp_msg);
            mvLog(MVLOG_DEBUG, "PROPOSE command completed");
            break;
        }
        case (Command::UPDATE):
        {
            mvLog(MVLOG_DEBUG, "UPDATE command received");
            DecoderRet decoder_ret = DecoderRet::SUCCESS;
            p_resp_msg->serialize(&decoder_ret, sizeof(DecoderRet));
            update(p_cmd_msg, p_resp_msg);
            mvLog(MVLOG_DEBUG, "UPDATE command completed");
            break;
        }
        default:
        {
            DecoderRet decoder_ret = DecoderRet::INVALID_CMD;
            p_resp_msg->serialize(&decoder_ret, sizeof(DecoderRet));
            mvLog(MVLOG_ERROR, "Invalid command received");
            break;
        }
    }
}

void VPUHostTimeSrv::propose(core::Message *p_cmd_msg, core::Message *p_resp_msg)
{
    mvLog(MVLOG_INFO, "Proposing host time");
    std::int64_t host_time_ns, los_time_ns;

    // Get LOS time
    timespec los_time;
    MvGetTimestamp(&los_time);
    MvTimespecToNanos(&los_time_ns, &los_time);

    // Get Host time
    p_cmd_msg->deserialize(&host_time_ns, sizeof(std::int64_t));

    // Save difference
    host_to_los_time_ns_candidate_ =
            host_time_ns - los_time_ns;

    mvLog(MVLOG_DEBUG, "Proposed host time is %lld (ns)", host_time_ns);
    mvLog(MVLOG_DEBUG, "Proposed host to los time is %lld (ns)",
            host_to_los_time_ns_candidate_);
}

void VPUHostTimeSrv::update(core::Message *p_cmd_msg, core::Message *p_resp_msg)
{
    mvLog(MVLOG_INFO, "Updating host time");
    std::int64_t adjust_host_time_ns, los_time_ns;
    p_cmd_msg->deserialize(&adjust_host_time_ns, sizeof(std::int64_t));
    host_to_los_time_ns_candidate_ += adjust_host_time_ns;
    timesync::save_host_to_los_time(host_to_los_time_ns_candidate_);
    mvLog(MVLOG_DEBUG, "Updated host to los time is %lld (ns)", host_to_los_time_ns_candidate_);
}

} // namespace hosttimesrv
} // namespace vpual
