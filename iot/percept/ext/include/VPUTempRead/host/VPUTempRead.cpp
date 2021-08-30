// Includes
// -------------------------------------------------------------------------------------
#include <VpualDispatcher.h>
#include "VPUTempRead.hpp"

#define MVLOG_UNIT_NAME VPUTempRead
#include <mvLog.h>

namespace vpual {
namespace tempread {
namespace stub {

// Functions implementation
// -------------------------------------------------------------------------------------
static core::Stub *pStub;

RetStatus_t Init()
{
    RetStatus_t vpu_ret = INIT_SUCCESS;

    // Stub constructor to create the corresponding decoder
    pStub = new core::Stub("TempRead");

    // Notify device
    uint8_t data = (uint8_t)action::INIT;
    core::Message cmd;
    cmd.serialize(&data, sizeof(data));
    core::Message rep;
    pStub->dispatch(cmd,rep);

    uint8_t act_check;
    rep.deserialize(&act_check, sizeof(act_check));
    if(0 != act_check)
    {
        mvLog(MVLOG_ERROR,"Decoder error. Action not accepted/implemented");
        vpu_ret = INIT_ERROR;
    }
    else
    {
        StatusCode_t init_resp;
        rep.deserialize(&init_resp, sizeof(init_resp));
        if(INIT_SUCCESS == init_resp.ret_status)
        {
            // mvLog(MVLOG_INFO,"Temperature reading initialized");
        }
        else
        {
            mvLog(MVLOG_ERROR, "Error %d during temperature sensors initialization", init_resp.ret_status);
            vpu_ret = (RetStatus_t) init_resp.ret_status;
        }
    }

    return vpu_ret;
}
RetStatus_t Get(float *css, float *mss, float *upa, float *dss)
{
    RetStatus_t vpu_ret = GET_SUCCESS;

    // Notify device
    uint8_t data = (uint8_t)action::GET_TEMPERATURE;
    core::Message cmd;
    cmd.serialize(&data, sizeof(data));
    core::Message rep;
    pStub->dispatch(cmd,rep);

    uint8_t act_check;
    rep.deserialize(&act_check, sizeof(act_check));
    if(0 != act_check)
    {
        mvLog(MVLOG_ERROR,"Decoder error. Action not accepted/implemented");
        vpu_ret = GET_ERROR;
    }
    else
    {
        Entry_t entry;
        StatusCode_t get_resp;
        rep.deserialize(&get_resp, sizeof(get_resp));
        if(GET_SUCCESS == get_resp.ret_status)
        {
            rep.deserialize(&entry, sizeof(Entry_t));
            *css = entry.value[IDX_CSS];
            *mss = entry.value[IDX_MSS];
            *upa = entry.value[IDX_UPA];
            *dss = entry.value[IDX_DSS];
        }
        else
        {
            *css = 0.0;
            *mss = 0.0;
            *upa = 0.0;
            *dss = 0.0;
            mvLog(MVLOG_ERROR," Error %d during temperature reading", get_resp.ret_status);
            vpu_ret = (RetStatus_t) get_resp.ret_status;
        }
    }

    return vpu_ret;
}

void Deinit()
{
    delete pStub;
}

} // namespace stub
} // namespace tempread
} // namespace vpual
