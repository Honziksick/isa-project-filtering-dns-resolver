/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsMessageParser.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessageParser.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "DnsUtils/DnsMessageParser.hpp"
#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/StringUtils.hpp"
#include <arpa/inet.h>  // ntohs()
#include <cstdint>      // uint8_t, uint16_t
#include <cstring>      // std::memcpy
#include <utility>      // std::move
#include <string>       // std::string

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsQuery DnsMessageParser::parseAndValidate(const uint8_t *pMessageBuffer,
                                                const size_t messageBufferLength) {
        // First we parse the DNS message header
        DnsHeader dnsHeader{};
        if(!parseHeader(pMessageBuffer, messageBufferLength, dnsHeader)) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "HEADER");
        }

        // We read first 12B from the buffer (header size)
        size_t offset = DnsQuery::HEADER_TRUE_SIZE;

        // Then we parse the domain name (QNAME) label by label
        string qname;
        if(!parseQName(pMessageBuffer, messageBufferLength, offset, qname)) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");  // If QNAME is malformed
        }

        // After QNAME, we parse QTYPE and QCLASS
        const uint16_t qtype = parseQType(pMessageBuffer, messageBufferLength, offset);
        const uint16_t qclass = parseQClass(pMessageBuffer, messageBufferLength, offset);

        // Last, we validate the parts of the parsed query
        validateDnsQuery(dnsHeader, qtype, qclass);

        return DnsQuery{dnsHeader, offset, move(qname), qtype, qclass};
    } // DnsMessageParser::parseAndValidate

    bool DnsMessageParser::parseHeader(const uint8_t *pMessageBuffer,
                                       const size_t messageBufferLength,
                                       DnsHeader &outDnsHeader) {
        // First we check if the header is complete
        if(messageBufferLength < DnsQuery::HEADER_TRUE_SIZE) {
            return false;
        }

        // Now we can safely parse the header fields we need
        outDnsHeader.mId = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::ID_MSB] << 8) |
            pMessageBuffer[DnsHeader::ID_LSB]
        );
        outDnsHeader.mFlags = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::FLAGS_MSB] << 8) |
            pMessageBuffer[DnsHeader::FLAGS_LSB]
        );
        outDnsHeader.mQdCount = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::QDCOUNT_MSB] << 8) |
            pMessageBuffer[DnsHeader::QDCOUNT_LSB]
        );
        return true;
    } // DnsMessageParser::parseHeader

    bool DnsMessageParser::parseQName(const uint8_t *pMessageBuffer,
                                      const size_t messageBufferLength,
                                      size_t &currentOffset,
                                      string &outQName) {
        // QNAME is a sequence of labels ending with a zero-length label (0 byte)
        while(true) {
            // If we reach the end of the buffer without finding a zero-length label, it's an error
            if(currentOffset >= messageBufferLength) {
                return false;
            }

            // Read the length of the next label
            const uint8_t labelLength = pMessageBuffer[currentOffset++];

            // If the length is zero, we've reached the end of the QNAME
            if(labelLength == 0) {
                break;
            }

            // Check for compression (not supported in this implementation)
            if((labelLength & 0xC0) != 0) {
                return false;
            }

            // If the label length exceeds the remaining buffer, it's an error
            if(currentOffset + labelLength > messageBufferLength) {
                return false;
            }

            // Append the label to the output QNAME and add a dot if it's not the first label
            if(!outQName.empty()) {
                outQName.push_back('.');
            }
            outQName.append(reinterpret_cast<const char*>(pMessageBuffer + currentOffset), labelLength);

            // Move the offset past the label
            currentOffset += labelLength;
        } // while(true)

        // Conversion to lowercase for case-insensitive comparison
        StringUtils::toLower(outQName);

        return true;
    } // DnsMessageParser::parseQName

    uint16_t DnsMessageParser::parseQType(const uint8_t *pMessageBuffer,
                                          const size_t messageBufferLength,
                                          size_t &currentOffset) {
        // First we check the bounds to prevent reading past the end of the received message
        if(currentOffset + DnsQuery::QTYPE_SIZE > messageBufferLength) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QTYPE");
        }

        // QTYPE is 2 bytes (big-endian)
        const auto qtype = static_cast<uint16_t>(
            (pMessageBuffer[currentOffset] << 8) | pMessageBuffer[currentOffset + 1]
        );

        // We move the offset past QTYPE
        currentOffset += DnsQuery::QTYPE_SIZE;
        return qtype;
    } // DnsMessageParser::parseQType

    uint16_t DnsMessageParser::parseQClass(const uint8_t *pMessageBuffer,
                                           const size_t messageBufferLength,
                                           size_t &currentOffset) {
        // First we check the bounds to prevent reading past the end of the received message
        if(currentOffset + DnsQuery::QCLASS_SIZE > messageBufferLength) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QCLASS");
        }

        // QCLASS is 2 bytes (big-endian)
        const auto qclass = static_cast<uint16_t>(
            (pMessageBuffer[currentOffset] << 8) | pMessageBuffer[currentOffset + 1]
        );

        // We move the offset past QCLASS
        currentOffset += DnsQuery::QCLASS_SIZE;
        return qclass;
    } // DnsMessageParser::parseQClass

    void DnsMessageParser::validateDnsQuery(const DnsHeader &dnsHeader,
                                            const uint16_t qtype,
                                            const uint16_t qclass) {
        // Check if QR bit is set (must be 0 for query)
        if(DnsHeader::isQRSet(dnsHeader.mFlags)) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QR");
        }

        // Check if RD bit is set (not supported)
        if(DnsHeader::isRDSet(dnsHeader.mFlags) != 0) {
            throw DnsParseErrorException(DnsRCodes::NOTIMP, "RD");
        }

        // Check if Z bit is zero (must be 0)
        if(DnsHeader::isZBitZero(dnsHeader.mFlags) != 0) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "ZBIT");
        }

        // Check that QDCOUNT is exactly 1
        if(dnsHeader.mQdCount != 1) {
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QDCOUNT");
        }

        // Only A IN queries (QTYPE=1, QCLASS=1) are allowed
        if(qtype != 1) {
            throw DnsParseErrorException(DnsRCodes::NOTIMP, "QTYPE");
        }
        if(qclass != 1) {
            throw DnsParseErrorException(DnsRCodes::NOTIMP, "QCLASS");
        }
    } // DnsMessageParser::validateQuery
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsMessageParser.cpp ***/
