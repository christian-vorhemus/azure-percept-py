/*
 * VPUHostTimeSrv.hpp
 *
 *  Created on: Sep 2, 2020
 *      Author: apalfi
 */

#ifndef VPUHOSTTIMESRV_LEON_VPUHOSTTIMESRV_HPP_
#define VPUHOSTTIMESRV_LEON_VPUHOSTTIMESRV_HPP_

// Includes
// ----------------------------------------------------------------------------
#include <Decoder.h>

namespace vpual
{
namespace hosttimesrv
{
// Defines
// ----------------------------------------------------------------------------
/**
 * @brief VPUHostTimeSrv decoder.
 */
class VPUHostTimeSrv final : public core::Decoder
{
public:
    VPUHostTimeSrv();

    void Decode(core::Message *p_cmd_msg, core::Message *p_resp_msg);

    ~VPUHostTimeSrv() = default;

private:

    /**
     * @brief FIXME
     *
     */
    void propose(core::Message *p_cmd_msg, core::Message *p_resp_msg);
    void update(core::Message *p_cmd_msg, core::Message *p_resp_msg);

    std::int64_t host_to_los_time_ns_candidate_ = 0;
};

} // namespace hosttimesrv
} // namespace vpual

#endif /* VPUHOSTTIMESRV_LEON_VPUHOSTTIMESRV_HPP_ */
