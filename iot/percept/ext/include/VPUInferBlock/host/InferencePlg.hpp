#ifndef INFERENCE_PLG_HPP_
#define INFERENCE_PLG_HPP_

#include <vector>
#include <deque>

#include <XLink.h>
#include <VpualDispatcher.h>
#include <VPUBlockTypes.h>
#include <VPUBlockXLink.h>

#include "InferenceIOInfo.hpp" // for vpual::infer::IOInfo

namespace vpual {
namespace stub {

class Inference : public core::Stub {
    // Supported Stub functionality
    enum class action : char {
        INIT = 0,
        LOAD_NETWORK,
        GET_INFO,
    };
public:
    Inference();

    void create();

    // Input and blob are moved, no need for additional copies
    void load_network(const std::vector<char>& blob) const;

    const vpual::infer::IOInfo& get_info();

    void push_input(const std::deque<Frame> & inputs);
    void pull_result(std::deque<Frame> * pOutputs);
private:
    streamId_t sid_;
    static constexpr std::size_t strm_size_ = 1 * 1024 * 1024;
    static constexpr std::size_t infer_in_strm_size_ = 10 * 1024 * 1024;

    // NN
    XLinkVpuIn inStream_;
    XLinkVpuOut outStream_;


    vpual::infer::IOInfo blob_info_;
};

} // namespace stub
} // namespace vpual

#endif // INFERENCE_PLG_HPP_
