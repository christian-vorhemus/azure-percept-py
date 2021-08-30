/*
 * CameraDecoder.cpp
 *
 *  Created on: Apr 20, 2020
 *      Author: apalfi
 */

// Includes
// -------------------------------------------------------------------------------------
#define MVLOG_UNIT_NAME CameraDecoder
#include <mvLog.h>

#include "CameraDecoder.h"
#include <vector>
#include <RmtUtilsInstance.h>
#include <RmtCache_Stub.h>
#include <RmtInstance_Stub.h>
#include <RmtPipeCameraBlockRT_Stub.h>

extern "C" {
#include <camera_control.h>
}
#include "VidEnc/app_videnc_cfg.h"
#include "app_pipe_mode_config.h"
#include <app_dewarp_mesh.h> //FIXME

// Defines
// -------------------------------------------------------------------------------------
#define ISP_LINE_STRIDE_ALIGN (32)
// FIXME: receive from App or Host
#define ISP_FIRST_SLICE      (8)  /// First cmx slice for SIPP isp (both color and mono)
#define ISP_COLOR_NROFSLICES (6)  /// Nr of slices for color ISP

// FIXME: move to dedicated header and cleanup
#define POOL_SRC_COUNT (3)
#define POOL_ISP_VDO_COUNT (4)
#define POOL_PPB_COUNT (4)
#define POOL_PPS_COUNT (15)
#define POOL_ENC_COUNT (5)
#define POOL_ENC_FSZ (3*1024*1024)

// Global data
// -------------------------------------------------------------------------------------

// Functions implementation
// -------------------------------------------------------------------------------------
namespace vpual {
namespace decoder {

CameraDecoder::CameraDecoder(const Configs & configs,
                             const Utils & utils,
                             const Resources & resources):
    configs{configs},
    utils{utils},
    resources{resources},
    vidEncSettings{},
    pPipeOS(nullptr),
    pPipeRT(nullptr)
{
    mvLogLevelSet(MVLOG_WARN);
    assert(configs.modes.size() != 0);
    assert(utils.pFrmPoolAlloc != nullptr);
    assert(utils.pRefKeeper != nullptr);
    assert(resources.pPpenc != nullptr);
}

void CameraDecoder::Decode(core::Message *cmd, core::Message *rep) {
    char command1;
    cmd->deserialize(&command1,sizeof(char));
    action command = (action)command1;
    std::vector<char> network;

    switch(command) {
        case action::INIT:
            mvLog(MVLOG_INFO,"Camera INIT command received");
            init();
            // rep->create(1); // Dummy; ToDo: send proper reply
            mvLog(MVLOG_INFO,"Camera INIT command completed");
            break;
        case action::START:
            mvLog(MVLOG_INFO,"Camera START command received");
            start(cmd);
            // rep->create(1); // Dummy; ToDo: send proper reply
            mvLog(MVLOG_INFO,"Camera START command completed");
            break;
        default:
            mvLog(MVLOG_ERROR,"Unknown command %d received",command);
            exit(1);
            break;
    }
    uint8_t response = 0;
    rep->serialize(&response,sizeof(response));
}

void CameraDecoder::init(void) {
    // Nothing left
}

void CameraDecoder::start(core::Message *cmd)
{
    CameraConfig cameraConfig;
    VidEncConfig vidEncConfig;
    cmd->deserialize(&cameraConfig.mode, sizeof(cameraConfig.mode));
    cmd->deserialize(&cameraConfig.fps, sizeof(cameraConfig.fps));
    cmd->deserialize(&vidEncConfig.enabled, sizeof(vidEncConfig.enabled));
    cmd->deserialize(&vidEncConfig.bitrate, sizeof(vidEncConfig.bitrate));
    cmd->deserialize(&vidEncConfig.framerate, sizeof(vidEncConfig.framerate));
    cmd->deserialize(&vidEncConfig.gopSize, sizeof(vidEncConfig.gopSize));
    mvLog(MVLOG_INFO,"Starting camera on mode %d at %d FPS", cameraConfig.mode, cameraConfig.fps);
    mvLog(MVLOG_INFO, "Video encoder parameters: bitrate: %u, framerate: %u, gop_size: %u",
            vidEncConfig.bitrate, vidEncConfig.framerate, vidEncConfig.gopSize);

    AppPipeConfigMode mode = (AppPipeConfigMode)(cameraConfig.mode-1); // FIXME
    assert(mode < configs.modes.size());

    // Instance OS and RT sides
    pPipeOS = rmt::utils::New<PipeCameraBlockOS>(&HeapAlloc);
    assert(pPipeOS != nullptr);
    pPipeRT = RMT_NEW(PipeCameraBlockRT);
    assert(pPipeRT != nullptr);
    rmt::PipeCameraBlockRTStub pipeRTStub;
    pipeRTStub.Bind(&pPipeRT->data);

    // Configure OS side
    std::string streamNamePreview = "CameraOutBgr" + std::to_string(id);
    std::string streamNameVideo = "CameraOutVidEnc" + std::to_string(id);
    std::string streamNameRelPreview = "CameraBgrRelease" + std::to_string(id);

    PipeCameraBlockOS::Configs osConfigs;
    osConfigs.camId = 0; //FIXME: provide as parameter
    osConfigs.ipcThreadPrio = 240;
    osConfigs.pRtRiSrcCtrl = &pPipeRT->data.rtRiSrcCtrl;
    osConfigs.pRtSeSrcCtrl = &pPipeRT->data.rtSeSrcCtrl;
    osConfigs.pRtSeEv = &pPipeRT->data.rtSeEv;
    osConfigs.pEventsMesageQueueName = "mqGuzziEvents";
    osConfigs.pRtSePreview = &pPipeRT->data.rtSePreview;
    osConfigs.pStreamNamePreview = streamNamePreview.c_str();
    osConfigs.sizeMaxPreview = 2 * (configs.modes[mode].ppencOutPrvWidth * configs.modes[mode].ppencOutPrvHeight * 3);
    osConfigs.pRtSeVideo = &pPipeRT->data.rtSeVideo;
    osConfigs.pStreamNameVideo = streamNameVideo.c_str();
    osConfigs.sizeMaxVideo = POOL_ENC_FSZ * 2;
    osConfigs.pStreamNameRelPreview = streamNameRelPreview.c_str();

    PipeCameraBlockOS::Utils osUtils;
    osUtils.pRefKeeper = utils.pRefKeeper;

    pPipeOS->data.Config(&osConfigs, &osUtils);


    // Configure RT side
    rmt::utils::CacheAligned<PipeCameraBlockRT::Configs> rtConfigs;

    rtConfigs.data.camId = 0; //FIXME: provide as parameter
    rtConfigs.data.srcCfg.nrFrmsSrc = POOL_SRC_COUNT;
    rtConfigs.data.srcCfg.maxImgSz = configs.modes[mode].srcWidth * configs.modes[mode].srcHeight * 5/4; // PACK10
    rtConfigs.data.ipcThreadPrio = 240;
    rtConfigs.data.pOsSeSrcCtrl = &pPipeOS->data.osSeSrcCtrl;
    rtConfigs.data.pOsRiSrcCtrl = &pPipeOS->data.osRiSrcCtrl;
    rtConfigs.data.pOsRiEv = &pPipeOS->data.osRiEv;
    rtConfigs.data.ispCfg.nrFrmsIsp = POOL_ISP_VDO_COUNT;
    rtConfigs.data.ispCfg.maxImgSz = ALIGN_UP(configs.modes[mode].ppencInWidth, ISP_LINE_STRIDE_ALIGN) *
                                     configs.modes[mode].ppencInHeight * 3/2; // YUV420
    rtConfigs.data.ispCfg.lineStrideAlign = ISP_LINE_STRIDE_ALIGN;
    rtConfigs.data.ispCfg.firstCmxSliceUsed = ISP_FIRST_SLICE;
    rtConfigs.data.ispCfg.nrOfCmxSliceUsed = ISP_COLOR_NROFSLICES;
    rtConfigs.data.colorIspType = (cameraConfig.mode == 1)?(PipeCameraBlockRT::Configs::ColorIspType::Native):
                                                           (PipeCameraBlockRT::Configs::ColorIspType::ScaleCrop);
    rtConfigs.data.scaleCrop.scaleFactors = configs.modes[mode].ispScaleFactors;
    rtConfigs.data.scaleCrop.cropWindow = configs.modes[mode].ispCropWindow;
    rtConfigs.data.postProcCfg.i_img_w = configs.modes[mode].ppencInWidth;
    rtConfigs.data.postProcCfg.i_img_h = configs.modes[mode].ppencInHeight;
    rtConfigs.data.postProcCfg.i_img_s = ALIGN_UP(configs.modes[mode].ppencInWidth, ISP_LINE_STRIDE_ALIGN);
    rtConfigs.data.postProcCfg.warp_cfg.mesh_w  = configs.modes[mode].warpMeshWidth;
    rtConfigs.data.postProcCfg.warp_cfg.mesh_h  = configs.modes[mode].warpMeshHeight;
    rtConfigs.data.postProcCfg.warp_cfg.mesh_base = (uint32_t *)App_DeWarpMesh_GetMesh(mode); //FIXME
    assert(rtConfigs.data.postProcCfg.warp_cfg.mesh_base != NULL);
    rtConfigs.data.postProcCfg.warp_cfg.eng_id_y = configs.modes[mode].warpLumaEngineId;
    rtConfigs.data.postProcCfg.warp_cfg.eng_id_c = configs.modes[mode].warpChromaEngineId;
    rtConfigs.data.postProcCfg.warp_cfg.pfbc_req_mode_y = configs.modes[mode].warpLumaPfbcReqMode;
    rtConfigs.data.postProcCfg.warp_cfg.pfbc_req_mode_c = configs.modes[mode].warpChromaPfbcReqMode;
    rtConfigs.data.postProcCfg.o_img_vdo_w = configs.modes[mode].ppencOutVdoWidth;
    rtConfigs.data.postProcCfg.o_img_vdo_h = configs.modes[mode].ppencOutVdoHeight;
    rtConfigs.data.postProcCfg.o_img_stl_w = configs.modes[mode].ppencOutStillWidth;
    rtConfigs.data.postProcCfg.o_img_stl_h = configs.modes[mode].ppencOutStillHeight;
    rtConfigs.data.postProcCfg.o_img_prv_w = configs.modes[mode].ppencOutPrvWidth;
    rtConfigs.data.postProcCfg.o_img_prv_h = configs.modes[mode].ppencOutPrvHeight;
    rtConfigs.data.postProcCfg.frmT = BGR888p;
    rtConfigs.data.postProcCfg.frm = CV_U8;
    rtConfigs.data.postProcCfg.mesh_bypass = 0;
    rtConfigs.data.postProcCfg.expose_crops_scalefacts.crop_ldmai.x = 0;
    rtConfigs.data.postProcCfg.expose_crops_scalefacts.crop_ldmai.y = 0;
    rtConfigs.data.postProcCfg.preview_distorsion = 0;
    rtConfigs.data.postProcRes = *resources.pPpenc;
    rtConfigs.data.poolCfgPreview.slotsNr = POOL_PPS_COUNT;
    rtConfigs.data.poolCfgPreview.slotSz = configs.modes[mode].ppencOutPrvWidth *
                                           configs.modes[mode].ppencOutPrvHeight * 3; // BGRp
    rtConfigs.data.enableVideo = static_cast<bool>(vidEncConfig.enabled);
    rtConfigs.data.poolCfgVideo.slotsNr = POOL_PPB_COUNT;
    rtConfigs.data.poolCfgVideo.slotSz = configs.modes[mode].ppencOutVdoWidth *
                                         configs.modes[mode].ppencOutVdoHeight * 3/2; // NV12
    rtConfigs.data.poolCfgStill.slotsNr = 0;
    rtConfigs.data.poolCfgStill.slotSz = 0;
    rtConfigs.data.nominalFps = cameraConfig.fps;
    GetBufSettings(&vidEncSettings.data, app_videnc_cfg);
    vidEncSettings.data.tChParam[0].uWidth = configs.modes[mode].vidEncFrmWidth;
    vidEncSettings.data.tChParam[0].uHeight = configs.modes[mode].vidEncFrmHeight;
    vidEncSettings.data.tChParam[0].tRCParam.uTargetBitRate = vidEncConfig.bitrate;
    vidEncSettings.data.tChParam[0].tRCParam.uMaxBitRate = vidEncConfig.bitrate;
    vidEncSettings.data.tChParam[0].tRCParam.uFrameRate = vidEncConfig.framerate;
    vidEncSettings.data.tChParam[0].tGopParam.uGopLength = vidEncConfig.gopSize;
    vidEncSettings.data.tChParam[0].tGopParam.uFreqIDR = vidEncConfig.framerate;
    rtConfigs.data.grpVidEncCfg.pVidEncSettings = &vidEncSettings;
    rtConfigs.data.grpVidEncCfg.poolCfgVidEnc.slotsNr = POOL_ENC_COUNT;
    rtConfigs.data.grpVidEncCfg.poolCfgVidEnc.slotSz = POOL_ENC_FSZ;
    rtConfigs.data.grpVidEncCfg.poolCfgVidEnc.shared = true;
    rtConfigs.data.pOsRiPreview = &pPipeOS->data.osRiPreview;
    rtConfigs.data.pOsRiVideo = &pPipeOS->data.osRiVideo;
    pipeRTStub.Config(&rtConfigs);

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

    // Start camera
    camera_control_start_with_fps(0, cameraConfig.fps);
    camera_control_focus_trigger(0);
    // ToDo: detect if IMX378 is connected
    // Enable this for continuous focus
    #if 0
    camera_control_af_mode(0, CAMERA_CONTROL__AF_MODE_CONTINUOUS_VIDEO);
    #endif
    mvLog(MVLOG_INFO,"Camera started successfully");
}

CameraDecoder::~CameraDecoder() {
    mvLog(MVLOG_INFO,"Camera stream closed successfully");
}

} // namespace decoder
} // namespace vpual
