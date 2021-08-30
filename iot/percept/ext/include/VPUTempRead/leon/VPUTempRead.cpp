// Includes
// -------------------------------------------------------------------------------------
#include "TempRead.h"
#include "VPUTempRead.h"

#define MVLOG_UNIT_NAME VPUTempRead
#include <mvLog.h>

namespace vpual {
namespace tempread {
namespace decoder {

// Functions implementation
// -------------------------------------------------------------------------------------
VPUTempRead::VPUTempRead() { mvLogLevelSet(MVLOG_WARN); }

void VPUTempRead::Decode(core::Message *cmd, core::Message *rep)
{
    char command1;
    cmd->deserialize(&command1, sizeof(char));
    action command = (action)command1;

    //ToDo: implement validity-check for commands, currently 0-OK, 1-NOK
    uint8_t response;

    switch(command) {
        case action::INIT:
            response = 0;
            rep->serialize(&response, sizeof(response));
            mvLog(MVLOG_INFO,"Temperature sensors INIT command received");
            init(rep);
            mvLog(MVLOG_INFO,"Temperature sensors INIT command completed");
            break;
        case action::GET_TEMPERATURE:
            response = 0;
            rep->serialize(&response, sizeof(response));
            mvLog(MVLOG_INFO,"GET Temperature command received");
            get(rep);
            mvLog(MVLOG_INFO,"GET Temperature command completed");
            break;
        default:
            response = 1;
            rep->serialize(&response, sizeof(response));
            mvLog(MVLOG_ERROR,"Unknown command %d received",command);
            break;
    }
}

StatusCode_t VPUTempRead::init(core::Message *reply) {

    StatusCode_t funcRet;

    funcRet.ret_code = ::tempread::Init();
    if(RTEMS_SUCCESSFUL == funcRet.ret_code)
    {
        mvLog(MVLOG_INFO, "Temperature sensors initialized successfully");
        funcRet.ret_status = INIT_SUCCESS;
    }
    else if (RTEMS_RESOURCE_IN_USE == funcRet.ret_code)
    {
        mvLog(MVLOG_ERROR, "Temperature sensors already initialized");
        funcRet.ret_status = ALREADY_INITIALIZED;
    }
    else
    {
        mvLog(MVLOG_ERROR, "Failed to init temperature sensors %d", funcRet.ret_code);
        funcRet.ret_status = INIT_ERROR;
    }

    reply->serialize(&funcRet, sizeof(StatusCode_t));

    return funcRet;
}

StatusCode_t VPUTempRead::get(core::Message *reply) {

    ::tempread::TempRead_Entry_t entry;
    Entry_t vpu_entry;
    StatusCode_t funcRet;

    funcRet.ret_code = ::tempread::Get(&entry);
    if(RTEMS_SUCCESSFUL == funcRet.ret_code)
    {
        for (int i = 0; i < IDX_MAX; i++) {
            vpu_entry.value[i] = entry.tempVal[i]; }
        funcRet.ret_status = GET_SUCCESS;
    }
    else if(RTEMS_INTERNAL_ERROR == funcRet.ret_code)
    {
        mvLog(MVLOG_ERROR, "Temperature sensors not initialized %d", funcRet.ret_code);
        funcRet.ret_status = NOT_INITIALIZED;
    }
    else
    {
        funcRet.ret_status = GET_ERROR;
        mvLog(MVLOG_ERROR, "Failed to Get Temperature %d", funcRet.ret_code);
    }

    reply->serialize(&funcRet, sizeof(StatusCode_t));
    if(GET_SUCCESS == funcRet.ret_status)
        reply->serialize(&vpu_entry, sizeof(Entry_t));

    return funcRet;
}

VPUTempRead::~VPUTempRead() {
    mvLog(MVLOG_INFO,"Temperature read end");
}

} // namespace decoder
} // namespace tempread
} // namespace vpual
