#pragma once

#include <cstdint> // for std::size_t

#include <XLink.h> // for streamId_t
#include "VpualMessage.h" // for vpual::core::Message

namespace utils {
namespace xlink {

/// Read available data from an XLink stream
/// Variable amount of data read from the first available packet/
/// Data must be released by calling XLinkReadData, when processed by consumers
///
/// \param id of the XLink stream
/// \param [out] size read from the stream
///
/// \returns a pointer to the packet data.
std::uint8_t* read_available(streamId_t id,std::size_t* size);

/// Read data from an XLink packet
/// At most specified maxsize data is guaranteed to be returned from a packet
///
/// \param id of the XLink stream
/// \param maxsize the size to be read from the stream
/// \param [out] data received from the stream of size maxsize
///
/// \returns the size of the read data
std::size_t read_channel(const streamId_t id, void *data,const std::size_t maxsize);

/// Read data from an XLink stream
/// At most specified maxsize data is guaranteed to be returned from one or
/// more packets.
///
/// \param id of the XLink stream
/// \param maxsize the size to be read from the stream
/// \param [out] data received from the stream of size maxsize
///
/// \returns the size of the read data
std::size_t read_channel_stream(const streamId_t id,void *data,const std::size_t maxsize);

// TODO: Replace return with std::error_code
bool send_message(streamId_t id,const vpual::core::Message& message);

// TODO: Replace return with std::error_code
vpual::core::Message receive_message(streamId_t id);

} // namespace xlink
} // namespace utils
