/*
 * VPUTelemetry_OS.h
 *
 *  Created on: Jan 19, 2021
 *      Author: apalfi
 */

#ifndef VPUTELEMETRY_LEON_OS_VPUTELEMETRY_OS_H_
#define VPUTELEMETRY_LEON_OS_VPUTELEMETRY_OS_H_

// Includes
// ----------------------------------------------------------------------------
#include "Decoder.h"

namespace vpual {
namespace telemetry {

// Defines
// ----------------------------------------------------------------------------

class VPUTelemetry final:public core::Decoder {

public:
    /** Decode Method. */
    void Decode(core::Message *request, core::Message *response) override;

private:
    void init(core::Message *response);
    void deinit(core::Message *response);
    void getMemory(core::Message *response);
};

} // namespace telemetry
} // namespace vpual

#endif /* VPUTELEMETRY_LEON_OS_VPUTELEMETRY_OS_H_ */
