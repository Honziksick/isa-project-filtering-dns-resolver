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
 * Last edit:    01.10.2025                                                    *
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
#include "Enums/DnsOpcodes.hpp"
#include <unordered_map>  // std::unordered_map
#include <string>         // std::string

using namespace std;

namespace FilteringDnsResolver::Enums::Mapping
{
    const unordered_map<ExitCodes, string> &EnumMaps::getExitCodesMap() {
        static const unordered_map<ExitCodes, string> cMap = {
            {ExitCodes::SUCCESS, "Success"},
            {ExitCodes::INTERNAL_ERROR, "Internal Error"},
            {ExitCodes::INVALID_ARGUMENT_ERROR, "Invalid Argument Error"},
            {ExitCodes::INVALID_FILTER_FILE_CONTENT_ERROR, "Invalid Filter File Content Error"},
            {ExitCodes::UNKNOWN_ERROR, "Unknown Error"},
            {ExitCodes::PROTOCOL_ERROR, "Protocol Error"},
            {ExitCodes::SOCKET_ERROR, "Socket Error"},
            {ExitCodes::CONNECTION_ERROR, "Connection Error"},
            {ExitCodes::HOSTNAME_RESOLUTION_ERROR, "Hostname Resolution Error"},
            {ExitCodes::USER_INTERRUPTION_ERROR, "User Interruption Error"}
        };
        return cMap;
    } // EnumMaps::getExitCodesMap

    const unordered_map<DnsOpcodes, string> &EnumMaps::getDnsOpcodesMap() {
        static const unordered_map<DnsOpcodes, string> cMap = {
            {DnsOpcodes::QUERY, "QUERY"},
            {DnsOpcodes::IQUERY, "IQUERY"},
            {DnsOpcodes::STATUS, "STATUS"},
            {DnsOpcodes::UNASSIGNED_3, "UNASSIGNED_3"},
            {DnsOpcodes::NOTIFY, "NOTIFY"},
            {DnsOpcodes::UPDATE, "UPDATE"},
            {DnsOpcodes::STATEFUL_OPS, "DNS_STATEFUL_OPERATIONS"},
            {DnsOpcodes::UNASSIGNED_7, "UNASSIGNED_7"},
            {DnsOpcodes::UNASSIGNED_8, "UNASSIGNED_8"},
            {DnsOpcodes::UNASSIGNED_9, "UNASSIGNED_9"},
            {DnsOpcodes::UNASSIGNED_10, "UNASSIGNED_10"},
            {DnsOpcodes::UNASSIGNED_11, "UNASSIGNED_11"},
            {DnsOpcodes::UNASSIGNED_12, "UNASSIGNED_12"},
            {DnsOpcodes::UNASSIGNED_13, "UNASSIGNED_13"},
            {DnsOpcodes::UNASSIGNED_14, "UNASSIGNED_14"},
            {DnsOpcodes::UNASSIGNED_15, "UNASSIGNED_15"}
        };
        return cMap;
    } // EnumMaps::getDnsOpcodesMap

    const unordered_map<DnsRCodes, string> &EnumMaps::getDnsRCodesMap() {
        static const unordered_map<DnsRCodes, string> cMap = {
            {DnsRCodes::NOERROR, "NOERROR"},
            {DnsRCodes::FORMERR, "FORMERR"},
            {DnsRCodes::SERVFAIL, "SERVFAIL"},
            {DnsRCodes::NXDOMAIN, "NXDOMAIN"},
            {DnsRCodes::NOTIMP, "NOTIMP"},
            {DnsRCodes::REFUSED, "REFUSED"},
            {DnsRCodes::YXDOMAIN, "YXDOMAIN"},
            {DnsRCodes::XRRSET, "XRRSET"},
            {DnsRCodes::NOTAUTH, "NOTAUTH"},
            {DnsRCodes::NOTZONE, "NOTZONE"}
        };
        return cMap;
    } // EnumMaps::getDnsRCodesMap
} // FilteringDnsResolver::Enums::Mapping

/*** end of file EnumMaps.cpp ***/
