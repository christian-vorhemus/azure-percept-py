/*
 * VPUTelemetry.h
 *
 *  Created on: Jan 15, 2021
 *      Author: apalfi
 */

#ifndef VPUTELEMETRY_HOST_VPUTELEMETRY_H_
#define VPUTELEMETRY_HOST_VPUTELEMETRY_H_

// Includes
// ----------------------------------------------------------------------------
#include <cstdint>
#include <map>
#include <string>

// Defines
// ----------------------------------------------------------------------------
namespace vpual {
namespace telemetry {

int Init();
int Deinit();

class Memory {
public:
    struct Entry {
        std::uint32_t used;
        std::uint32_t total;
    };

    struct Info {
        Entry poolMain;
        Entry losHeap;
        Entry lrtHeap;
    };
    static std::int32_t Get(Info * pInfo);
    static void Print(const Info & info);
};

} // namespace telemetry
} // namespace vpual

#endif /* VPUTELEMETRY_HOST_VPUTELEMETRY_H_ */
