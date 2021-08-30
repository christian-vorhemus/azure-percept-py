///
/// @file      Dispatcher.h
/// @copyright All code copyright Movidius Ltd 2018, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Header for the VPUAL Dispatcher.
///
#pragma once

#include <string> // for std::string
#include <vector> // for std::vector
#include <cstdint> // for std::uint32_t

#include <XLink.h> // for XLinkHandler_t
#include <VpualMessage.h> // for vpual::core::Message

// Get the XLink device handle for VPUAL dispatcher.
//
// @param - XLink device ID (unused now; added for plugin migration)
//
// @return - XLink device handle.
//
XLinkHandler_t getXlinkDeviceHandler(std::uint32_t device_id);

namespace vpual {
namespace core {

// VPUAL start method
//
// @return - 0 on success
//
// Starts the VPUAL Dispatcher and creates an XLink channel
int start();

// VPUAL stop method
//
// @return - 0 on success
//
// Stops the VPUAL Dispatcher
int stop();

//
// Base class for all Stubs.
//
// Handles stub creation, destruction and communication
//
class Stub {
protected:
    std::uint32_t id_; // ID of the stub and matching decoder.
public:
    // Constructor
    // @param type - the string name of the decoder type to create.
    Stub(std::string type, std::uint32_t device_id = 0);

    // Destroy this stub and the corresponding decoder on the device.
    virtual ~Stub() noexcept(false);

    // ID getter
    std::uint32_t get_id() const;

    // Dispatches a message to the device and receives a response
    // @param cmd - message to be dispatched
    // @param rep - message to be received
    void dispatch(const Message& cmd, Message& rep) const;

    // Dispatches a message to the device
    // @param msg - message to be dispatched
    void dispatch_req(const Message& msg) const;

    // Receives a response from device
    // @param rep - message to be received
    void dispatch_resp(Message& rep) const;

    // Explicitly defaulted copy semantics
    Stub(const Stub&) = default;
    Stub& operator=(const Stub&) = default;

    // Explicitly defaulted move semantics
    Stub(Stub&&) = default;
    Stub& operator=(Stub&&) = default;

    // *********************************************************** //
    // Plugin Migration API
    // *********************************************************** //

    //
    // Dispatch.
    // Dispatch a command to the corresponding decoder on the device and wait
    // for a response.
    //
    // @param cmd The "command" message to dispatch to the decoder.
    // @param rep The "response" message containing the reply from the decoder.
    //
    // Added for plugin migration
    //
    void VpualDispatch(const VpualMessage *const cmd, VpualMessage* rep);

    //
    // Get device ID.
    //
    // @return device_id (0 for MX version)
    //
    // Added for plugin migration
    //
    std::uint32_t getDeviceId() const;

    std::uint32_t stubID; // ID of the stub and matching decoder.
private:
    // Header is fixed and must not be modified during send
    void send_header(const CommandHeader& cmd) const;

    // Low level command and response abstractions
    // Parameters passed as pointers, since memory does not require ownership
    void send_command(const void* command,std::size_t cmd_size) const;
    void get_response(Message& rep) const;
};

} // namespace core
} // namespace vpual

typedef vpual::core::Stub VpualStub;

