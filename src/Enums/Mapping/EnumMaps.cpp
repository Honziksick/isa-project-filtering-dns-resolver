/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         EnumMaps.cpp                                                  *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Implementation of the `EnumMaps` class, which provides        *
 *               static mapping utilities for converting enum values to        *
 *               their corresponding string representations.                   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file EnumMaps.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of static mapping utilities for enum-to-string
 *        conversions.
 */

#include "Enums/Mapping/EnumMaps.hpp"
#include "Enums/ExitCodes.hpp"
#include <unordered_map>  // std::unordered_map
#include <string>         // std::string

using namespace std;

namespace FilteringDNSResolver::Enums::Mapping
{
    const unordered_map<ExitCodes, string> &EnumMaps::getExitCodesMap() {
        static const unordered_map<ExitCodes, string> cMap = {
            {ExitCodes::SUCCESS, "Success"},
            {ExitCodes::INTERNAL_ERROR, "Internal Error"},
            {ExitCodes::INVALID_ARGUMENT_ERROR, "Invalid Argument Error"},
            {ExitCodes::UNKNOWN_ERROR, "Unknown Error"},
            {ExitCodes::PROTOCOL_ERROR, "Protocol Error"},
            {ExitCodes::HOSTNAME_RESOLUTION_ERROR, "Hostname Resolution Error"},
            {ExitCodes::USER_INTERRUPTION_ERROR, "User Interruption Error"}
        };
        return cMap;
    } // EnumMaps::getExitCodesMap
} // FilteringDNSResolver::Enums::Mapping

/*** end of file EnumMaps.cpp ***/
