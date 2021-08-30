// Includes
//-----------------------------------------------------------------------------
#define MVLOG_UNIT_NAME InferenceDecoder
#include <mvLog.h>

#include "InferenceDecoder.h"

#include <string.h>
#include <rtems.h>

#include <GraphManager.hpp>

#include "xlink_utils.hpp"

#include <RmtUtilsCache.h>
#include <RmtUtilsInstance.h>
#include <RmtCache_Stub.h>
#include <RmtInstance_Stub.h>
#include <PipeInferBlockOS.h>
#include <RmtPipeInferBlockRT_Stub.h>

// Defines
//-----------------------------------------------------------------------------
// ROI extractor
#define POOL_ROIMESH_W     (2)
#define POOL_ROIMESH_H     (2)
#define POOL_ROIMESH_FSZ   (POOL_ROIMESH_W * POOL_ROIMESH_H * 2 * sizeof(float))
#define POOL_ROIMESH_COUNT (2)

// NN Processing
#define POOL_TENSOR_OUT_COUNT  (2)

namespace vpual {
namespace decoder {

// Functions implementation
//-----------------------------------------------------------------------------
InferenceDecoder::InferenceDecoder(const Utils & utils) :
    sid_{INVALID_STREAM_ID},
    blob_{NULL},
    blob_size_{0},
    utils{utils},
    pPipeOS{NULL},
    pPipeRT{NULL}
{
    mvLogLevelSet(MVLOG_WARN);
    assert(utils.pRefKeeper != nullptr);
}

void InferenceDecoder::Decode(core::Message *cmd, core::Message *rep) {
    char command1;
    cmd->deserialize(&command1,sizeof(command1));
    action command = (action)command1;
    std::vector<char> network;

    switch(command) {
        case action::INIT:
            mvLog(MVLOG_INFO,"Inference INIT command received");
            init();
            // Send created response
            rep->serialize("Inference stream on",sizeof("Inference stream on"));
            mvLog(MVLOG_INFO,"Inference INIT command completed");
            break;
        case action::LOAD_NETWORK:
            mvLog(MVLOG_INFO,"Inference LOAD_NETWORK command received");
            load_network();
            rep->serialize("Inference received blob",sizeof("Inference received blob"));
            mvLog(MVLOG_INFO,"Inference LOAD_NETWORK command completed");
            break;
        case action::GET_INFO:
            mvLog(MVLOG_INFO,"Inference SEND_INFO command received");
            send_info(rep);
            mvLog(MVLOG_INFO,"Inference SEND_INFO command completed");
            break;
        default:
            mvLog(MVLOG_ERROR,"Unknown command %d received",command);
            exit(1);
            break;
    }
}

void InferenceDecoder::init(void) {
    if (sid_ != INVALID_STREAM_ID) {
        mvLog(MVLOG_ERROR,"Inference Stream already opened");
        return;
    }

    // TODO: Replace with linkid struct
    // TODO: update with shared header
    std::string chanNameStream = "InferenceStream" + std::to_string(id);
    sid_ = XLinkOpenStream(0, chanNameStream.c_str(), strm_size_); // remove hardcode
    if (sid_ == INVALID_STREAM_ID || sid_ == INVALID_STREAM_ID_OUT_OF_MEMORY) {
        mvLog(MVLOG_ERROR,"Could not open inference stream");
        exit(1);
    }

    mvLog(MVLOG_INFO,"Opened Inference stream successfully");
}

/*
    A network is expected in the form of a header and the actual blob
    The network header contains its size and is used for pool region allocation
*/
void InferenceDecoder::load_network(void) {
    mvLog(MVLOG_INFO,"Read network header");

    std::size_t net_size = 0;
    std::size_t received = utils::xlink::read_channel(sid_,&net_size,sizeof(std::size_t));

    mvLog(MVLOG_INFO,"Reading blob of size %zu",net_size);

    blob_ = GraphManager::get().get_region(net_size);
    mvLog(MVLOG_INFO,"Read network header successfully");

    mvLog(MVLOG_INFO,"Reading network from stream id = %lu", sid_);

    received = 0;
    received = utils::xlink::read_channel_stream(sid_,blob_.get(),net_size);
    if (received > net_size) {
        mvLog(MVLOG_ERROR,"[FATAL] Received larger network than expected");
        exit(1);
    }
    rtems_cache_flush_multiple_data_lines((const void *)blob_.get(), net_size);

    mvLog(MVLOG_INFO,"received = %zu",received);

    mvLog(MVLOG_INFO,"Network read successfully");

    // Get IO info
    info.data = GraphManager::get_io_info(blob_.get(), net_size);

    // Instance OS and RT sides
    pPipeOS = rmt::utils::New<PipeInferBlockOS>(&HeapAlloc);
    assert(pPipeOS != nullptr);
    pPipeRT = RMT_NEW(PipeInferBlockRT);
    assert(pPipeRT != nullptr);
    rmt::PipeInferBlockRTStub pipeRTStub;
    pipeRTStub.Bind(&pPipeRT->data);

    // Configure OS side
    std::string inChanName = "InferIn" + std::to_string(id);
    std::string outChanName = "InferOut" + std::to_string(id);

    PipeInferBlockOSCfg osCfg;
    osCfg.pInChanName = inChanName.c_str();
    osCfg.ipcThreadPrio = 240;
    osCfg.pRtRiIn = &pPipeRT->data.rtRiIn;
    osCfg.pRtSeOut = &pPipeRT->data.rtSeOut;
    osCfg.pOutChanName = outChanName.c_str();
    osCfg.outSizeMax = DCALGN(info.data.total_output_length);

    PipeInferBlockOSUtils osUtils;
    osUtils.pRefKeeper = utils.pRefKeeper;

    pPipeOS->data.Config(&osCfg, &osUtils);

    // Configure RT side
    PlgWarpHWCfg roiWarpHWCfg =
    {
            4,                    // availableWarpEngines [WARP2] // FIXME: provide as parameters
            1,                    // useOPIPE
            0,                    // useDMAIn
            1,                    // useDMAOut
            14, // ROI_WARP_FIRST_SLICE // CMXStartSlice // FIXME: provide as parameters
            1,  // ROI_WARP_NROFSLICES // CMXNoOfSlices
            0,                    // filterMode [Bilinear]
            CropOff,              // cropMode
            0,                    // cropHor
            0,                    // cropVert
            0x4,                  // pfbcReqMode
            0x0,                  // tileXPrefLog2
            0x0                   // tileYPrefLog2
    };

    rmt::utils::CacheAligned<PipeInferBlockRTCfg> rtCfg;
    rtCfg.data.ipcThreadPrio = 240;
    rtCfg.data.pOsSeIn = &pPipeOS->data.osSeIn;
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiMeshCfg.slotsNr = POOL_ROIMESH_COUNT;
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiMeshCfg.slotSz = POOL_ROIMESH_FSZ;
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiMeshCfg.shared = false;
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiCfg.slotsNr = info.data.inputs_count * 2;
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiCfg.slotSz =
        DCALGN(*std::max_element(info.data.input_lengths.begin(),
                                 info.data.input_lengths.end()));
    rtCfg.data.grpInferCfg.roiExtractCfg.poolRoiCfg.shared = false;
    rtCfg.data.grpInferCfg.roiExtractCfg.roiWarpCfg = roiWarpHWCfg;
    rtCfg.data.grpInferCfg.pBlob = blob_.get();
    rtCfg.data.grpInferCfg.blobLen = net_size;
    rtCfg.data.grpInferCfg.poolTensorCfg.slotsNr = POOL_TENSOR_OUT_COUNT;
    rtCfg.data.grpInferCfg.poolTensorCfg.slotSz = DCALGN(info.data.total_output_length);
    rtCfg.data.grpInferCfg.poolTensorCfg.shared = true;
    rtCfg.data.pOsRiOut = &pPipeOS->data.osRiOut;
    pipeRTStub.Config(&rtCfg);

    // Create pipe
    pPipeOS->data.Create();
    rmt::cacheFlush(pPipeOS, sizeof(*pPipeOS));
    rmt::RemoteCacheInvalidate(pPipeOS, sizeof(*pPipeOS));
    pipeRTStub.Create();
    rmt::RemoteCacheFlush(pPipeRT, sizeof(*pPipeRT));
    rmt::cacheInvalidate(pPipeRT, sizeof(*pPipeRT));

    // Start pipe
    pPipeOS->data.Start();
    pipeRTStub.Start();
}

void InferenceDecoder::send_info(core::Message* info_msg) const {
    assert(sid_ != INVALID_STREAM_ID && "Inference Stream is not open!");

    info_msg->serialize((std::uint32_t)info.data.inputs_count);

    for (const auto& input : info.data.input_names) {
        info_msg->serialize((std::uint32_t)input.length());
        info_msg->serialize(input.c_str(),input.length());
    }

    // Serialize input shapes
    for (const auto& ishape : info.data.input_shapes) {
        info_msg->serialize(&ishape.layout, sizeof(ishape.layout));
        info_msg->serialize(&ishape.data_type, sizeof(ishape.data_type));
        info_msg->serialize(&ishape.num_dims, sizeof(ishape.num_dims));

        info_msg->serialize(ishape.dims.data(),
                ishape.dims.size() * sizeof(std::uint32_t));
    }

    info_msg->serialize((std::uint32_t)info.data.outputs_count);

    for (const auto& output : info.data.output_names) {
        info_msg->serialize((std::uint32_t)output.length());
        info_msg->serialize(output.c_str(),output.length());
    }

    // Serialize output shapes
    for (const auto& oshape : info.data.output_shapes) {
        info_msg->serialize(&oshape.layout, sizeof(oshape.layout));
        info_msg->serialize(&oshape.data_type, sizeof(oshape.data_type));
        info_msg->serialize(&oshape.num_dims, sizeof(oshape.num_dims));

        info_msg->serialize(oshape.dims.data(),
                oshape.dims.size() * sizeof(std::uint32_t));
    }

    // Serialize output tensor offsets
    info_msg->serialize(info.data.output_offsets.data(),
        info.data.output_offsets.size() * sizeof(std::uint32_t));

    // Serialize output tensor lengths
    info_msg->serialize(info.data.output_lengths.data(),
        info.data.output_lengths.size() * sizeof(std::uint32_t));

    // Serialize total output tensors length
    info_msg->serialize((std::uint32_t)info.data.total_output_length);
}

InferenceDecoder::~InferenceDecoder() {
#if 0 // ToDo: Check this
    if (XLinkCloseStream(sid_)) {
        mvLog(MVLOG_WARN,"Inference stream could not be closed");
    }
    mvLog(MVLOG_INFO,"Inference stream closed successfully");
#endif
}

} // namespace decoder
} // namespace vpual
