#pragma once

#include "Decoder.h"
#include "VPUTempReadTypes.h"

namespace vpual {
namespace tempread {
namespace decoder {

class VPUTempRead final:public core::Decoder {

public:

    VPUTempRead();

    /** Decode Method. */
    void Decode(core::Message *cmd, core::Message *rep) override;

    ~VPUTempRead();
private:
    StatusCode_t init(core::Message *reply);
    StatusCode_t get(core::Message *repply);
};

} // namespace decoder
} // namespace tempread
} // namespace vpual
