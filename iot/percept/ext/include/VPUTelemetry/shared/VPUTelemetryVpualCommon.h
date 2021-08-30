/*
 * VPUTelemetryVpualCommon.h
 *
 *  Created on: Jan 19, 2021
 *      Author: apalfi
 */

#ifndef VPUTELEMETRY_SHARED_VPUTELEMETRYVPUALCOMMON_H_
#define VPUTELEMETRY_SHARED_VPUTELEMETRYVPUALCOMMON_H_

// Include
// ----------------------------------------------------------------------------

namespace vpual
{
namespace telemetry
{
// Exported Types
// ----------------------------------------------------------------------------
/**
 * @brief Command ID requested by Host.
 */
enum class Command : std::uint8_t
{
    INIT,
    DEINIT,
    GETMEMORY,
};

} // namespace telemetry
} // namespace vpual

#endif /* VPUTELEMETRY_SHARED_VPUTELEMETRYVPUALCOMMON_H_ */
