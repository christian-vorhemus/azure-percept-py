#include "xlink_utils.hpp"

#include <cassert> // for assert
#include <cstring> // for std::std::memcpy
#include <cstdint> // for std::size, std::std::uint8_t, std::std::uint32_t

#define MVLOG_UNIT_NAME xlink_utils
#include <mvLog.h>

namespace utils {
namespace xlink {

std::uint8_t* read_available(streamId_t id,std::size_t* size) {
    streamPacketDesc_t* packet;

    int status = XLinkReadData(id, &packet);
    if (status != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR,"Data could not be read");
        return nullptr;
    } else if(packet == NULL) {
        mvLog(MVLOG_ERROR,"Received empty packet");
        return nullptr;
    }

    if (size)
        *size = packet->length;
    return packet->data;
}

std::size_t read_channel_stream(const streamId_t id,void *data,const std::size_t maxsize) {
    mvLogLevelSet(MVLOG_ERROR);

    std::uint32_t offset = 0;
    std::uint32_t readSize = 0;
    std::uint32_t remaining_size = maxsize;
    do {
        readSize = read_channel(id,(void*) ((char*)data + offset),remaining_size);
        // ToDo: check readSize not 0
        offset += readSize;
        remaining_size = maxsize - offset;
    } while (remaining_size > 0);
    return offset;
}

std::size_t read_channel(const streamId_t id,void *data,const std::size_t maxsize) {
    mvLogLevelSet(MVLOG_ERROR);

    std::size_t read_len = 0;
    streamPacketDesc_t * packet;

    int status = XLinkReadData(id, &packet);
    if (status != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR,"Data could not be read");
        return 0;
    } else if(packet->length > maxsize) {
        mvLog(MVLOG_ERROR,"Received too much data; overflow");
        return 0;
    }
    else
    {
        // ToDo: add LEON L2 cache handling
        std::memcpy((std::uint8_t*)data, packet->data, packet->length);
        read_len = packet->length;
    }
    status = XLinkReleaseData(id);
    if (status != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR,"Release data failed");
        return 0;
    }

    return read_len;
}

bool send_message(streamId_t id, const vpual::core::Message& message) {
    auto status = XLinkWriteData(id,message.data(),message.size());
    return status != X_LINK_SUCCESS;
}

vpual::core::Message receive_message(streamId_t id) {
    std::size_t size = 0;
    auto* data = read_available(id,&size);
    if (data == nullptr) {
        mvLog(MVLOG_ERROR,"Received nullptr message data");
        assert(0);
    }

    vpual::core::Message message {};
    message.serialize(data,size);

    XLinkReleaseData(id);

    return message;
}

} // namespace xlink
} // namespace utils
