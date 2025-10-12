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
 * Description:  This source file implements the `DnsMessageParser` class,     *
 *               which implements parsing and validation of DNS query messages *
 *               according to RFC 1035. It provides functionality for          *
 *               extracting DNS header fields, QNAME domain names, QTYPE and   *
 *               QCLASS values from raw message buffers with comprehensive     *
 *               validation of message format and protocol compliance.         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessageParser.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DnsMessageParser` class for DNS message
 *        parsing and validation functionality.
 */

#include "DnsUtils/DnsMessageParser.hpp"
#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/StringUtils.hpp"
#include "Utilities/Logger.hpp"
#include <cstdint>  // uint8_t, uint16_t
#include <utility>  // std::move
#include <string>   // std::string

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsQuery DnsMessageParser::parseAndValidate(const uint8_t *pMessageBuffer,
                                                const size_t messageBufferLength) {
        logger("DnsMessageParser::parseAndValidate() called with buffer=%p, length=%zu bytes",
               static_cast<const void*>(pMessageBuffer), messageBufferLength);
        verbose("Parsing incoming DNS query message (%zu bytes)", messageBufferLength);

        // First we parse the DNS message header
        DnsHeader dnsHeader{};
        logger("Attempting to parse DNS header from buffer");
        if(!parseHeader(pMessageBuffer, messageBufferLength, dnsHeader)) {
            logger("DNS header parsing failed - throwing FORMERR for HEADER");
            verbose("DNS message header is malformed");
            throw DnsParseErrorException(DnsRCodes::FORMERR, "HEADER");
        }
        logger("DNS header successfully parsed: ID=%u, flags=0x%04X, qdcount=%u",
               dnsHeader.mId, dnsHeader.mFlags, dnsHeader.mQdCount);

        // We read first 12B from the buffer (header size)
        size_t offset = DnsQuery::HEADER_TRUE_SIZE;
        logger("Starting QNAME parsing at offset %zu (after %u-byte header)",
               offset, DnsQuery::HEADER_TRUE_SIZE);

        // Then we parse the domain name (QNAME) label by label
        string qname{};
        if(!parseQName(pMessageBuffer, messageBufferLength, offset, qname)) {
            logger("QNAME parsing failed at offset %zu - throwing FORMERR", offset);
            verbose("DNS query domain name is malformed");
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");  // If QNAME is malformed
        }
        logger("QNAME successfully parsed: '%s', final offset=%zu", qname.c_str(), offset);
        verbose("Parsed domain name: %s", qname.c_str());

        // After QNAME, we parse QTYPE and QCLASS
        logger("Parsing QTYPE at offset %zu", offset);
        const uint16_t qtype = parseQType(pMessageBuffer, messageBufferLength, offset);
        logger("QTYPE parsed: %u, new offset=%zu", qtype, offset);

        logger("Parsing QCLASS at offset %zu", offset);
        const uint16_t qclass = parseQClass(pMessageBuffer, messageBufferLength, offset);
        logger("QCLASS parsed: %u, final offset=%zu", qclass, offset);

        // Last, we validate the parts of the parsed query
        logger("Starting DNS query validation");
        validateDnsQuery(dnsHeader, qtype, qclass);
        logger("DNS query validation completed successfully");
        verbose("DNS query validation passed - query type %u, class %u", qtype, qclass);

        logger("Creating DnsQuery object with parsed data");
        return DnsQuery{dnsHeader, offset, move(qname), qtype, qclass};
    } // DnsMessageParser::parseAndValidate

    bool DnsMessageParser::parseHeader(const uint8_t *pMessageBuffer,
                                       const size_t messageBufferLength,
                                       DnsHeader &outDnsHeader) {
        logger("DnsMessageParser::parseHeader() called with buffer length %zu", messageBufferLength);

        // First we check if the header is complete
        if(messageBufferLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Header parsing failed: buffer too short (%zu < %u bytes)",
                   messageBufferLength, DnsQuery::HEADER_TRUE_SIZE);
            return false;
        }
        logger("Buffer length check passed (%zu >= %u bytes)",
               messageBufferLength, DnsQuery::HEADER_TRUE_SIZE);

        // Now we can safely parse the header fields we need
        outDnsHeader.mId = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::ID_MSB] << 8) |
            pMessageBuffer[DnsHeader::ID_LSB]
        );
        logger("Parsed ID from bytes [%u][%u]: %u (0x%04X)",
               pMessageBuffer[DnsHeader::ID_MSB], pMessageBuffer[DnsHeader::ID_LSB],
               outDnsHeader.mId, outDnsHeader.mId);

        outDnsHeader.mFlags = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::FLAGS_MSB] << 8) |
            pMessageBuffer[DnsHeader::FLAGS_LSB]
        );
        logger("Parsed FLAGS from bytes [%u][%u]: 0x%04X",
               pMessageBuffer[DnsHeader::FLAGS_MSB], pMessageBuffer[DnsHeader::FLAGS_LSB],
               outDnsHeader.mFlags);

        outDnsHeader.mQdCount = static_cast<uint16_t>(
            (pMessageBuffer[DnsHeader::QDCOUNT_MSB] << 8) |
            pMessageBuffer[DnsHeader::QDCOUNT_LSB]
        );
        logger("Parsed QDCOUNT from bytes [%u][%u]: %u",
               pMessageBuffer[DnsHeader::QDCOUNT_MSB], pMessageBuffer[DnsHeader::QDCOUNT_LSB],
               outDnsHeader.mQdCount);

        logger("Header parsing completed successfully");
        return true;
    } // DnsMessageParser::parseHeader

    bool DnsMessageParser::parseQName(const uint8_t *pMessageBuffer,
                                      const size_t messageBufferLength,
                                      size_t &currentOffset,
                                      string &outQName) {
        logger("DnsMessageParser::parseQName() called at offset %zu", currentOffset);
        const size_t startOffset = currentOffset;
        size_t labelCount = 0;

        // QNAME is a sequence of labels ending with a zero-length label (0 byte)
        while(true) {
            // If we reach the end of the buffer without finding a zero-length label, it's an error
            if(currentOffset >= messageBufferLength) {
                logger("QNAME parsing failed: reached buffer end at offset %zu without null terminator",
                       currentOffset);
                return false;
            }

            // Read the length of the next label
            const uint8_t labelLength = pMessageBuffer[currentOffset++];
            logger("Read label length: %u at offset %zu", labelLength, currentOffset - 1);

            // If the length is zero, we've reached the end of the QNAME
            if(labelLength == 0) {
                logger("Found QNAME terminator (0-length label) at offset %zu", currentOffset - 1);
                break;
            }

            // Check for compression (not supported in this implementation)
            if((labelLength & 0xC0) != 0) {
                logger("QNAME parsing failed: compression detected (label length 0x%02X has compression bits set)",
                       labelLength);
                return false;
            }

            // If the label length exceeds the remaining buffer, it's an error
            if(currentOffset + labelLength > messageBufferLength) {
                logger("QNAME parsing failed: label length %u exceeds remaining buffer (%zu + %u > %zu)",
                       labelLength, currentOffset, labelLength, messageBufferLength);
                return false;
            }

            // Append the label to the output QNAME and add a dot if it's not the first label
            if(!outQName.empty()) {
                outQName.push_back('.');
                logger("Added dot separator to QNAME");
            }

            string currentLabel(reinterpret_cast<const char*>(pMessageBuffer + currentOffset), labelLength);
            logger("Extracting label %zu: '%s' (length %u) from offset %zu",
                   labelCount, currentLabel.c_str(), labelLength, currentOffset);
            outQName.append(currentLabel);
            labelCount++;

            // Move the offset past the label
            currentOffset += labelLength;
            logger("Advanced offset to %zu after reading label", currentOffset);
        } // while(true)

        logger("QNAME before case conversion: '%s'", outQName.c_str());

        // Conversion to lowercase for case-insensitive comparison
        StringUtils::toLower(outQName);

        logger("QNAME parsing completed: '%s' (%zu labels, %zu total bytes processed)",
               outQName.c_str(), labelCount, currentOffset - startOffset);

        return true;
    } // DnsMessageParser::parseQName

    uint16_t DnsMessageParser::parseQType(const uint8_t *pMessageBuffer,
                                          const size_t messageBufferLength,
                                          size_t &currentOffset) {
        logger("DnsMessageParser::parseQType() called at offset %zu", currentOffset);

        // First we check the bounds to prevent reading past the end of the received message
        if(currentOffset + DnsQuery::QTYPE_SIZE > messageBufferLength) {
            logger("QTYPE parsing failed: insufficient buffer space (%zu + %u > %zu)",
                   currentOffset, DnsQuery::QTYPE_SIZE, messageBufferLength);
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QTYPE");
        }

        // QTYPE is 2 bytes (big-endian)
        const auto qtype = static_cast<uint16_t>(
            (pMessageBuffer[currentOffset] << 8) | pMessageBuffer[currentOffset + 1]
        );
        logger("Parsed QTYPE from bytes [%u][%u]: %u at offset %zu",
               pMessageBuffer[currentOffset], pMessageBuffer[currentOffset + 1], qtype, currentOffset);


        // We move the offset past QTYPE
        currentOffset += DnsQuery::QTYPE_SIZE;
        logger("Advanced offset to %zu after QTYPE", currentOffset);

        return qtype;
    } // DnsMessageParser::parseQType

    uint16_t DnsMessageParser::parseQClass(const uint8_t *pMessageBuffer,
                                           const size_t messageBufferLength,
                                           size_t &currentOffset) {
        logger("DnsMessageParser::parseQClass() called at offset %zu", currentOffset);

        // First we check the bounds to prevent reading past the end of the received message
        if(currentOffset + DnsQuery::QCLASS_SIZE > messageBufferLength) {
            logger("QCLASS parsing failed: insufficient buffer space (%zu + %u > %zu)",
                   currentOffset, DnsQuery::QCLASS_SIZE, messageBufferLength);
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QCLASS");
        }

        // QCLASS is 2 bytes (big-endian)
        const auto qclass = static_cast<uint16_t>(
            (pMessageBuffer[currentOffset] << 8) | pMessageBuffer[currentOffset + 1]
        );
        logger("Parsed QCLASS from bytes [%u][%u]: %u at offset %zu",
               pMessageBuffer[currentOffset], pMessageBuffer[currentOffset + 1], qclass, currentOffset);

        // We move the offset past QCLASS
        currentOffset += DnsQuery::QCLASS_SIZE;
        logger("Advanced offset to %zu after QCLASS", currentOffset);

        return qclass;
    } // DnsMessageParser::parseQClass

    void DnsMessageParser::validateDnsQuery(const DnsHeader &dnsHeader,
                                            const uint16_t qtype,
                                            const uint16_t qclass) {
        logger("DnsMessageParser::validateDnsQuery() called with ID=%u, flags=0x%04X, qtype=%u, qclass=%u",
               dnsHeader.mId, dnsHeader.mFlags, qtype, qclass);

        // Check if QR bit is set (must be 0 for query)
        logger("Validating QR bit in flags 0x%04X", dnsHeader.mFlags);
        if(DnsHeader::isQRSet(dnsHeader.mFlags)) {
            logger("Validation failed: QR bit is set (this is a response, not a query)");
            verbose("Received DNS response instead of query");
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QR");
        }
        logger("QR bit validation passed (query message confirmed)");

        // Check if Z bit is zero (must be 0)
        logger("Validating Z bit in flags 0x%04X", dnsHeader.mFlags);
        if(!DnsHeader::isZBitZero(dnsHeader.mFlags)) {
            logger("Validation failed: Z bit is not zero (reserved bit must be 0)");
            verbose("DNS query contains invalid reserved bits");
            throw DnsParseErrorException(DnsRCodes::FORMERR, "ZBIT");
        }
        logger("Z bit validation passed (reserved bit is zero)");

        // Check that QDCOUNT is exactly 1
        logger("Validating QDCOUNT: expected=1, actual=%u", dnsHeader.mQdCount);
        if(dnsHeader.mQdCount != 1) {
            logger("Validation failed: QDCOUNT is %u (must be exactly 1)", dnsHeader.mQdCount);
            verbose("DNS query contains %u questions (only single questions supported)", dnsHeader.mQdCount);
            throw DnsParseErrorException(DnsRCodes::FORMERR, "QDCOUNT");
        }
        logger("QDCOUNT validation passed");

        // Only A IN queries (QTYPE=1, QCLASS=1) are allowed
        // logger("Validating QTYPE: expected=1 (A record), actual=%u", qtype);
        // if(qtype != 1) {
        //     logger("Validation failed: QTYPE=%u (only A records supported)", qtype);
        //     verbose("Unsupported query type %u (only A records are supported)", qtype);
        //     throw DnsParseErrorException(DnsRCodes::NOTIMP, "QTYPE");
        // }
        // logger("QTYPE validation passed (A record query)");
        //
        // logger("Validating QCLASS: expected=1 (IN), actual=%u", qclass);
        // if(qclass != 1) {
        //     logger("Validation failed: QCLASS=%u (only IN class supported)", qclass);
        //     verbose("Unsupported query class %u (only Internet class is supported)", qclass);
        //     throw DnsParseErrorException(DnsRCodes::NOTIMP, "QCLASS");
        // }
        // logger("QCLASS validation passed (Internet class)");

        const uint16_t opcode = DnsHeader::getOpcode(dnsHeader.mFlags);
        logger("Validating OPCODE: expected=0 (QUERY), actual=%u", opcode);
        if(opcode != 0) {
            logger("Validation failed: OPCODE=%u (only standard queries supported)", opcode);
            verbose("Unsupported DNS operation code %u (only standard queries are supported)", opcode);
            throw DnsParseErrorException(DnsRCodes::NOTIMP, "OPCODE");
        }
        logger("OPCODE validation passed (standard query)");

        logger("All DNS query validations completed successfully");
    } // DnsMessageParser::validateQuery
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsMessageParser.cpp ***/
