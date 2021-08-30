/*
 * XLinkWrap_Host.hpp
 *
 *  Created on: Aug 12, 2020
 *      Author: apalfi
 */

#ifndef XLINKWRAP_HOST_XLINKWRAP_HOST_HPP_
#define XLINKWRAP_HOST_XLINKWRAP_HOST_HPP_

// Includes
// ----------------------------------------------------------------------------------------
#include <stdint.h>
#include <string>

#include <XLink.h>

namespace xlinkwrap {
namespace host {

// Defines
// ----------------------------------------------------------------------------------------
class Device
{
public:
    int32_t boot(const std::string& mvcmd_path);
    int32_t reset();

private:
    deviceDesc_t deviceDesc_;
    XLinkHandler_t deviceHandle_;
};

} // namespace host
} // namespace xlinkwrap

#endif /* XLINKWRAP_HOST_XLINKWRAP_HOST_HPP_ */
