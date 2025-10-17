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
#include "DnsUtils/DomainValidators.hpp"
#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/StringUtils.hpp"
#include "Utilities/CastUtils.hpp"
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
    void DnsMessageParser::parseAndValidate(const uint8_t *pMessageBuffer,
                                            const size_t messageBufferLength,
                                            DnsQuery &outQuery) {
        logger("DnsMessageParser::parseAndValidate() called with buffer=%p, length=%zu bytes",
               static_cast<const void*>(pMessageBuffer), messageBufferLength);
        verbose("Parsing incoming DNS query message (%zu bytes)", messageBufferLength);

        // Preparation of variables to hold parsed data
        DnsHeader dnsHeader{};
        string qname{};
        uint16_t qtype{0};
        uint16_t qclass{0};
        auto rcode{DnsRCodes::NOERROR};
        string errorDetail{};

        // First we parse the DNS message header
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
        if(!parseQName(pMessageBuffer, messageBufferLength, offset, qname)) {
            logger("QNAME parsing failed at offset %zu - throwing FORMERR", offset);
            verbose("DNS query domain name is malformed: %s", qname.c_str());

            rcode = DnsRCodes::FORMERR;
            errorDetail = "QNAME";
        }
        else {
            logger("QNAME successfully parsed: '%s', final offset=%zu", qname.c_str(), offset);
            verbose("Parsed domain name: %s", qname.c_str());
        }

        if(rcode == DnsRCodes::NOERROR) {
            // After QNAME, we parse QTYPE and QCLASS
            logger("Parsing QTYPE at offset %zu", offset);
            try {
                qtype = parseQType(pMessageBuffer, messageBufferLength, offset);
                logger("QTYPE parsed: %u, new offset=%zu", qtype, offset);
            }
            catch(const DnsParseErrorException &e) {
                logger("QTYPE parsing failed at offset %zu", offset);
                rcode = CastUtils::castIntToEnum<DnsRCodes>(e.code());
                errorDetail = e.detail();
            }

            if(rcode == DnsRCodes::NOERROR) {
                logger("Parsing QCLASS at offset %zu", offset);
                try {
                    qclass = parseQClass(pMessageBuffer, messageBufferLength, offset);
                    logger("QCLASS parsed: %u, final offset=%zu", qclass, offset);
                }
                catch(const DnsParseErrorException &e) {
                    logger("QCLASS parsing failed at offset %zu", offset);
                    rcode = CastUtils::castIntToEnum<DnsRCodes>(e.code());
                    errorDetail = e.detail();
                }
            } // if QNAME and QTYPE parsed successfully
        } // if QNAME parsed successfully

        logger("Creating DnsQuery object with parsed data (may be partial if errors occurred)");
        outQuery = DnsQuery{dnsHeader, offset, move(qname), qtype, qclass};

        // Last, we validate the parts of the parsed query
        if(rcode == DnsRCodes::NOERROR) {
            logger("Starting DNS query validation");
            try {
                validateDnsQuery(dnsHeader, qtype, qclass);
                logger("DNS query validation completed successfully");
                verbose("DNS query validation passed - query type %u, class %u", qtype, qclass);
            }
            catch(const DnsParseErrorException &e) {
                logger("DNS query validation failed: %s", e.detail().c_str());
                rcode = CastUtils::castIntToEnum<DnsRCodes>(e.code());
                errorDetail = e.detail();
            }
        } // if no errors so far
        else {
            logger("Previous parsing errors detected, validation will be skipped");
        }

        if(rcode != DnsRCodes::NOERROR) {
            logger("Errors detected during parsing/validation - throwing exception with RCODE=%d (%s)",
                   CastUtils::castEnumToInt(rcode), CastUtils::castEnumToString<DnsRCodes>(rcode).c_str());
            throw DnsParseErrorException(rcode, errorDetail);
        }
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
                                      size_t &inOutOffset,
                                      string &outQName) {
        logger("DnsMessageParser::parseQName() called at offset %zu", inOutOffset);
        const size_t inOffset{inOutOffset};
        size_t currentOffset{inOutOffset};
        size_t labelCount{0};

        // Preparation for potentially parsing compressed names
        bool usedCompression{false};
        size_t jumpCounter{0};
        size_t moveOffsetAfterJump{0};
        vector<bool> visited(messageBufferLength, false);

        // QNAME is a sequence of labels ending with a zero-length label (0 byte)
        while(true) {
            // If we reach the end of the buffer without finding a zero-length label, it's an error
            if(currentOffset >= messageBufferLength) {
                logger("QNAME parsing failed: reached buffer end at offset %zu without null terminator",
                       currentOffset);
                return false;
            }

            // Read the length of the next label or 1st byte of a compression pointer
            const uint8_t labelLengthOrPtr = pMessageBuffer[currentOffset++];
            logger("Read label length or pointer byte: 0x%02X at offset %zu", labelLengthOrPtr, currentOffset - 1);

            // If the length is zero, we've reached the end of the QNAME
            if(labelLengthOrPtr == 0) {
                logger("Found QNAME terminator (0-length label) at offset %zu", currentOffset - 1);

                // If we used compression, we need to adjust the original offset differerently
                if(!usedCompression) {
                    inOutOffset = currentOffset;
                }
                else {
                    inOutOffset = inOffset + moveOffsetAfterJump;
                }
                break;
            }

            // Check for compression
            // RFC 1035: The pointer takes the form of a two octet (2 + 14 bits):
            // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            // | 1  1|                OFFSET                   |
            // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            if((labelLengthOrPtr & DnsQuery::QNAME_COMPRESSION_MASK) == DnsQuery::QNAME_COMPRESSION_MASK) {
                logger("Interpreted as compression pointer (0x%02X)", labelLengthOrPtr);

                if(currentOffset >= messageBufferLength) {
                    logger("QNAME parsing failed: truncated compression pointer at end of buffer");
                    return false;
                }

                // We prepare everything needed for extracting the 14b pointer offset
                const uint8_t pointerLowerByte = pMessageBuffer[currentOffset++];
                constexpr uint8_t GET_SIX_LOWER_BITS_OF_BYTE{0b00111111};

                // Extract the 14-bit pointer offset - lowe6 6 bits from first byte + all 8 bits from second byte
                const uint16_t pointerOffset = static_cast<uint16_t>((labelLengthOrPtr & GET_SIX_LOWER_BITS_OF_BYTE) << 8) |
                        pointerLowerByte;
                logger("Compression pointer detected: target offset=%u (bytes: 0x%02X 0x%02X)",
                       pointerOffset, labelLengthOrPtr, pointerLowerByte);

                // Validate the pointer offset
                if(pointerOffset >= messageBufferLength) {
                    logger("QNAME parsing failed: pointer target %u out of bounds (len=%zu)",
                           pointerOffset, messageBufferLength);
                    return false;
                }

                // After the first compression jump, we remember how much to move the original offset
                if(!usedCompression) {
                    moveOffsetAfterJump = (currentOffset - inOffset);
                    logger("First compression jump: will advance 'currentOffset' by %zu bytes", moveOffsetAfterJump);

                    usedCompression = true; // flag that we used compression
                }

                jumpCounter++; // to avoid excessive jumps (before the check below)

                // Detect compression loops and excessive jumps
                if(jumpCounter > MAX_POINTER_CHAIN) {
                    logger("QNAME parsing failed: too many compression jumps (>%zu)", MAX_POINTER_CHAIN);
                    return false;
                }
                if(visited[pointerOffset]) {
                    logger("QNAME parsing failed: compression loop detected at offset %u", pointerOffset);
                    return false;
                }

                visited[pointerOffset] = true;  // mark this offset as visited to detect loops (after the checks above)

                // We jump to the pointer target offset (dots are added later)
                currentOffset = pointerOffset;
            } // if compression
            // Else it's a normal label (uncompressed)
            else {
                logger("Interpreted as uncompressed label of length: %u", labelLengthOrPtr);

                // Check for invalid top bits pattern (10xx xxxx) – invalid according to RFC 1035
                if((labelLengthOrPtr & 0b11000000) == 0b10000000) {
                    logger("QNAME parsing failed: invalid label length pattern 0x%02X (10xxxxxx)", labelLengthOrPtr);
                    return false;
                }

                // If the label length exceeds the remaining buffer, it's an error
                if(currentOffset + labelLengthOrPtr > messageBufferLength) {
                    logger("QNAME parsing failed: label length %u exceeds remaining buffer (%zu + %u > %zu)",
                           labelLengthOrPtr, currentOffset, labelLengthOrPtr, messageBufferLength);
                    return false;
                }

                // Append the label to the output QNAME and add a dot if it's not the first label
                if(!outQName.empty()) {
                    outQName.push_back('.');
                    logger("Added dot separator to QNAME");
                }

                string currentLabel(reinterpret_cast<const char*>(pMessageBuffer + currentOffset), labelLengthOrPtr);
                logger("Extracting label %zu: '%s' (length %u) from offset %zu",
                       labelCount, currentLabel.c_str(), labelLengthOrPtr, currentOffset);
                outQName.append(currentLabel);
                labelCount++;

                // Move the offset past the label
                currentOffset += labelLengthOrPtr;
                logger("Advanced offset to %zu after reading label", currentOffset);
            } // else uncompressed label
        } // while(true)

        logger("QNAME before case conversion: '%s'", outQName.c_str());

        // Conversion to lowercase for case-insensitive comparison
        StringUtils::toLower(outQName);

        // Remove trailing dot if present (just in case)
        bool hadTrailingDot{false};
        if(!outQName.empty() && outQName.back() == '.') {
            outQName.pop_back();
            hadTrailingDot = true;
            logger("Trailing dot removed from QNAME");
        }

        // Validate overall QName length and allowed characters
        try {
            logger("Validating QName length");
            DomainValidators::validateDomainLength(outQName, true);
            logger("QName length validation passed");

            logger("Validating QName characters");
            DomainValidators::validateDomainCharacters(outQName, true);
            logger("QName characters validation passed");

            // Split the QName into labels and validate each label
            logger("Splitting QName into labels and validating each");
            DomainValidators::splitByDotAndValidateLabels(outQName, true);
            logger("Label validation completed successfully");
        }
        catch(const DnsParseErrorException &e) {
            logger("QName validation failed: %s", e.detail().c_str());

            // If we removed a trailing dot, we can add it back for error reporting
            if(hadTrailingDot) {
                outQName.push_back('.');
                logger("Restored trailing dot to QNAME for error reporting");
            }
            return false;
        }
        catch(const exception &e) {
            logger("Unexpected error during QName validation: %s", e.what());

            // If we removed a trailing dot, we can add it back for error reporting
            if(hadTrailingDot) {
                outQName.push_back('.');
                logger("Restored trailing dot to QNAME for error reporting");
            }
            throw;  // Re-throw unexpected exceptions
        }

        logger("QNAME parsing completed: '%s' (%zu labels, %zu total bytes processed)",
               outQName.c_str(), labelCount, usedCompression ? moveOffsetAfterJump : (inOutOffset - inOffset));

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
