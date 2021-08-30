/*
 * CameraDecoder.h
 *
 *  Created on: Apr 20, 2020
 *      Author: apalfi
 */

#ifndef PROJECT_SHARED_VPUAL_BOARD_LEON_DECODERS_CAMERADECODER_H_
#define PROJECT_SHARED_VPUAL_BOARD_LEON_DECODERS_CAMERADECODER_H_

// Includes
// -------------------------------------------------------------------------------------
#include <cstdint>
#include <vector>
#include <Decoder.h>
#include <RmtUtilsCache.h>
#include <PipeCameraBlockOS.h>
#include <PipeCameraBlockRT.h>

extern "C" {
#include <PolyFirUtils.h>
}

// Classes
// -------------------------------------------------------------------------------------
namespace vpual {
namespace decoder {

class CameraDecoder final : public core::Decoder {

    // TODO: This should be shared between host & device (currently duplicated)
    // Supported Decoder functionality
    enum class action : char {
        INIT,
        // ToDo: improve serialization
        START,
    };

public:
    struct ModeConfigs {
        std::uint32_t srcWidth;
        std::uint32_t srcHeight;
        YuvScaleFactors ispScaleFactors;
        icRect ispCropWindow;
        icRect warpMeshCropWindow;
        uint32_t warpMeshWidth;
        uint32_t warpMeshHeight;
        uint32_t warpSrcMeshWidth;
        uint32_t warpSrcMeshHeight;
        uint32_t warpSrcMeshImgWidth;
        uint32_t warpSrcMeshImgHeight;
        uint32_t warpLumaPfbcReqMode;
        uint32_t warpChromaPfbcReqMode;
        uint32_t warpLumaEngineId;
        uint32_t warpChromaEngineId;
        std::uint32_t ppencInWidth;
        std::uint32_t ppencInHeight;
        std::uint32_t ppencOutVdoWidth;
        std::uint32_t ppencOutVdoHeight;
        std::uint32_t ppencOutStillWidth;
        std::uint32_t ppencOutStillHeight;
        std::uint32_t ppencOutPrvWidth;
        std::uint32_t ppencOutPrvHeight;
        std::uint32_t vidEncFrmWidth;
        std::uint32_t vidEncFrmHeight;
    };

    struct Configs {
        std::vector<ModeConfigs> modes;
    };

    struct Utils {
        IAllocator * pFrmPoolAlloc;
        RefKeeper * pRefKeeper;
    };
    struct Resources {
        ResourcesAlloc * pPpenc;
    };

    CameraDecoder() = delete;
    CameraDecoder(const Configs & configs,
                  const Utils & utils,
                  const Resources & resources);

    /** Decode Method. */
    void Decode(core::Message *cmd, core::Message *rep) override;

    bool operator==(const CameraDecoder& dec) { return this->id == dec.id; }

    ~CameraDecoder();
private:
    void init(void);
    void start(core::Message *cmd);

    struct CameraConfig {
        std::uint8_t mode;
        std::uint32_t fps;
    };

    struct VidEncConfig
    {
        std::uint8_t enabled;
        std::uint32_t bitrate;
        std::uint16_t framerate;
        std::uint16_t gopSize;
    };

    Configs configs;
    Utils utils;
    Resources resources;

    rmt::utils::CacheAligned<AL_TEncSettings> vidEncSettings;
    rmt::utils::CacheAligned<PipeCameraBlockOS> * pPipeOS;
    rmt::utils::CacheAligned<PipeCameraBlockRT> * pPipeRT;
};

} // namespace decoder
} // namespace vpual


#endif /* PROJECT_SHARED_VPUAL_BOARD_LEON_DECODERS_CAMERADECODER_H_ */
