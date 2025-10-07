/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsMessageParser.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessageParser.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_MESSAGE_PARSER_HPP
#define DNS_MESSAGE_PARSER_HPP

#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include <cstdint>  // uint8_t, uint16_t
#include <string>   // std::string

namespace FilteringDnsResolver::DnsUtils
{
    class DnsMessageParser {
    public:
        static DnsQuery parseAndValidate(const uint8_t *pMessageBuffer, size_t messageBufferLength);

    private:
        static bool parseHeader(const uint8_t *pMessageBuffer, size_t messageBufferLength, DnsHeader &outDnsHeader);
        static bool parseQName(const uint8_t *pMessageBuffer, size_t messageBufferLength, size_t &currentOffset, std::string &outQName);
        static uint16_t parseQType(const uint8_t *pMessageBuffer, size_t messageBufferLength, size_t &currentOffset);
        static uint16_t parseQClass(const uint8_t *pMessageBuffer, size_t messageBufferLength, size_t &currentOffset);
        static void validateDnsQuery(const DnsHeader &dnsHeader, uint16_t qtype, uint16_t qclass);
    }; // DnsMessageParser
} // FilteringDnsResolver::DNS

#endif // DNS_MESSAGE_PARSER_HPP

/*** end of file DnsMessageParser.hpp ***/
