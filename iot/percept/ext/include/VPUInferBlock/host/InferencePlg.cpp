#include "InferencePlg.hpp"
#include "InferenceIOInfo.hpp"

#include <vector> // for std::vector
#include <cassert> // for assert

#include <xlink_utils.hpp>
#include <PlgXlinkShared.h>
#include <secure_functions.h>

#define MVLOG_UNIT_NAME InferencePlg
#include <mvLog.h>

namespace vpual {
namespace stub {

Inference::Inference() : Stub("InferencePlugin") { mvLogLevelSet(MVLOG_WARN); }

void Inference::create() {
    auto xLinkHandler = getXlinkDeviceHandler(0);

    // open xlink stream for inference transfers
    std::string chanNameStream = "InferenceStream" + std::to_string(id_);
    sid_ = XLinkOpenStream(xLinkHandler.linkId, chanNameStream.c_str(), strm_size_);

    if (sid_ == INVALID_STREAM_ID || sid_ == INVALID_STREAM_ID_OUT_OF_MEMORY) {
        mvLog(MVLOG_FATAL,"Inference stream could not be opened!");
        exit(1);
    }

    // Open XLink streams
    std::string streamName;
    streamName = "InferIn" + std::to_string(Stub::id_);
    inStream_.Open(xLinkHandler.linkId, streamName.c_str(), infer_in_strm_size_);
    streamName = "InferOut" + std::to_string(Stub::id_);
    outStream_.Open(xLinkHandler.linkId, streamName.c_str());

    // Notify device to open stream
    uint8_t data = (uint8_t)action::INIT;
    core::Message cmd;
    cmd.serialize(&data,sizeof(data));
    core::Message rep;
    dispatch(cmd,rep);

    mvLog(MVLOG_INFO,"Inference stream opened");
}

const vpual::infer::IOInfo& Inference::get_info(void) {
    if (blob_info_.is_empty()) {
        mvLog(MVLOG_INFO,"Getting graph information from device");

        uint8_t data = (uint8_t)action::GET_INFO;
        core::Message cmd;
        core::Message info_msg;

        cmd.serialize(&data,sizeof(data));
        dispatch(cmd,info_msg);

        info_msg.deserialize(&blob_info_.inputs_count,
                sizeof(blob_info_.inputs_count));

        blob_info_.input_names.resize(blob_info_.inputs_count);

        for (std::size_t i = 0; i < blob_info_.inputs_count; i++) {
            std::uint32_t name_len = 0;
            info_msg.deserialize(&name_len,sizeof(name_len));

            blob_info_.input_names[i].resize(name_len);
            info_msg.deserialize(&blob_info_.input_names[i][0],name_len);
        }

        blob_info_.input_shapes.resize(blob_info_.inputs_count);
        for (std::size_t i = 0; i < blob_info_.inputs_count; i++) {

            info_msg.deserialize(&blob_info_.input_shapes[i].layout,
                    sizeof(blob_info_.input_shapes[i].layout));

            info_msg.deserialize(&blob_info_.input_shapes[i].data_type,
                sizeof(blob_info_.input_shapes[i].data_type));

            info_msg.deserialize(&blob_info_.input_shapes[i].num_dims,
                sizeof(blob_info_.input_shapes[i].num_dims));

            auto num_dims = blob_info_.input_shapes[i].num_dims;
            blob_info_.input_shapes[i].dims.resize(num_dims);

            info_msg.deserialize(blob_info_.input_shapes[i].dims.data(),
                num_dims * sizeof(std::uint32_t));
        }

        // Get output information
        info_msg.deserialize(&blob_info_.outputs_count,
                sizeof(blob_info_.outputs_count));

        blob_info_.output_names.resize(blob_info_.outputs_count);

        for (std::size_t i = 0; i < blob_info_.outputs_count; i++) {
            std::uint32_t name_len = 0;
            info_msg.deserialize(&name_len,sizeof(name_len));

            blob_info_.output_names[i].resize(name_len);
            info_msg.deserialize(&blob_info_.output_names[i][0],name_len);
        }

        blob_info_.output_shapes.resize(blob_info_.outputs_count);
        for (std::size_t i = 0; i < blob_info_.outputs_count; i++) {
            info_msg.deserialize(&blob_info_.output_shapes[i].layout,
                    sizeof(blob_info_.output_shapes[i].layout));

            info_msg.deserialize(&blob_info_.output_shapes[i].data_type,
                sizeof(blob_info_.output_shapes[i].data_type));

            info_msg.deserialize(&blob_info_.output_shapes[i].num_dims,
                sizeof(blob_info_.output_shapes[i].num_dims));

            auto num_dims = blob_info_.output_shapes[i].num_dims;
            blob_info_.output_shapes[i].dims.resize(num_dims);

            info_msg.deserialize(blob_info_.output_shapes[i].dims.data(),
                num_dims * sizeof(std::uint32_t));
        }

        blob_info_.output_offsets.resize(blob_info_.outputs_count);
        info_msg.deserialize(blob_info_.output_offsets.data(),
                blob_info_.outputs_count * sizeof(std::uint32_t));

        blob_info_.output_lengths.resize(blob_info_.outputs_count);
        info_msg.deserialize(blob_info_.output_lengths.data(),
                blob_info_.outputs_count * sizeof(std::uint32_t));

        info_msg.deserialize(&blob_info_.total_output_length,
                sizeof(blob_info_.total_output_length));

        mvLog(MVLOG_INFO,"Received IO Information from Device");
    }

    return blob_info_;
}



// TODO: Add command for blob written
void Inference::load_network(const std::vector<char>& network) const {
    mvLog(MVLOG_INFO,"Loading network with size %zu to device",network.size());

    // Notify blob loaded
    uint8_t data = (uint8_t)action::LOAD_NETWORK;
    core::Message cmd;
    cmd.serialize(&data,sizeof(data));
    dispatch_req(cmd);

    uint32_t netsize = network.size(); // 32 bits enough for all networks
    auto rc = XLinkWriteData(sid_,(uint8_t*)&netsize,sizeof(uint32_t));
    if (rc != X_LINK_SUCCESS) {
        mvLog(MVLOG_FATAL,"Inference stream: Could not write network size");
        exit(1); // TODO: proper error handling
    }

    mvLog(MVLOG_INFO,"Network header sent successfully");

    std::size_t written = 0;
    XLinkError_t xlink_ret = X_LINK_SUCCESS;
    while (((netsize - written) > 0) && (xlink_ret == X_LINK_SUCCESS))
    {
        std::size_t write_size;
        if ((netsize - written) > strm_size_)
            write_size = strm_size_;
        else
            write_size = netsize - written;

        xlink_ret = XLinkWriteData(sid_, reinterpret_cast<const uint8_t*>(network.data()) + written, write_size);

        written += write_size;
    }

    // TODO
    core::Message reply;
    dispatch_resp(reply);
    mvLog(MVLOG_INFO,"Network loaded on device");
}

void Inference::push_input(const std::deque<Frame> & inputs)
{
    mvLog(MVLOG_INFO,"Sending network inputs");

    // Send every input separately
    for (std::size_t i = 0; i < inputs.size(); i++)
    {
        inStream_.Write(inputs[i]);
    }

    mvLog(MVLOG_INFO,"Network inputs sent");
}

void Inference::pull_result(std::deque<Frame> * pOutputs)
{
    mvLog(MVLOG_INFO,"Receiving network results");

    std::deque<BufferSegment> segments;

    // Prepare every segment corresponding to network outputs
    for (std::size_t i = 0; i < blob_info_.outputs_count; i++)
    {
        BufferSegment segment;

        segment.offset = blob_info_.output_offsets[i];
        segment.size = blob_info_.output_lengths[i];

        segments.push_back(segment);
    }

    // Read all network outputs
    outStream_.Read(segments, pOutputs);

    mvLog(MVLOG_INFO,"Network results received");
}

} // namespace stub
} // namespace vpual
