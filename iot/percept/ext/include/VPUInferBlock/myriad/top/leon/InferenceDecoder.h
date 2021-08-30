#pragma once

// Includes
//-----------------------------------------------------------------------------
#include "XLink.h"
#include "Decoder.h"

#include <map>
#include <vector>
#include <memory>

#include <GraphManager.hpp>
#include <RmtUtilsCache.h>
#include <PipeInferBlockOS.h>
#include <PipeInferBlockRT.h>

namespace vpual {
namespace decoder {

// Defines
//-----------------------------------------------------------------------------
class InferenceDecoder final : public core::Decoder {
    friend class GraphManager;

    // TODO: This should be shared between host & device (currently duplicated)
    // Supported Decoder functionality
    enum class action : char {
        INIT,
        LOAD_NETWORK,
        GET_INFO,
    };

    streamId_t sid_;
    static constexpr std::size_t strm_size_ = 1 * 1024 * 1024;

    std::unique_ptr<char[],std::function<void(char*)>> blob_;
    std::size_t blob_size_;

public:
    struct Utils {
        RefKeeper * pRefKeeper;
    };

    InferenceDecoder() = delete;
    InferenceDecoder(const Utils & utils);

    /** Decode Method. */
    void Decode(core::Message *cmd, core::Message *rep) override;

    bool operator==(const InferenceDecoder& dec) { return this->id == dec.id; }
    ~InferenceDecoder();
protected:
    const char* get_blob(void) { return blob_.get(); }
    std::size_t get_id(void) { return id; }
private:
    void init(void);
    void load_network(void);
    void send_info(core::Message* info_msg) const;

    Utils utils;

    rmt::utils::CacheAligned<PipeInferBlockOS> * pPipeOS;
    rmt::utils::CacheAligned<PipeInferBlockRT> * pPipeRT;
    rmt::utils::CacheAligned<GraphManager::IOInfo> info;
};

} // namespace decoder
} // namespace vpual
