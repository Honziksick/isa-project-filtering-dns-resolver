/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsMessenger.cpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    07.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `DnsMessenger` class, which   *
 *               handles sending DNS responses and error messages to clients.  *
 *               It provides functionality for constructing and sending        *
 *               various DNS response types including successful replies       *
 *               and error responses with appropriate RCODE values according   *
 *               to RFC 1035.                                                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessenger.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DnsMessenger` class for DNS response
 *        construction and transmission functionality.
 */

#include "DnsUtils/DnsMessenger.hpp"
#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsMessageParser.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/CastUtils.hpp"
#include "Utilities/Logger.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <arpa/inet.h>   // inet_ntoa, ntohs
#include <sys/socket.h>  // sendto
#include <algorithm>     // std::min
#include <cstring>       // memcpy
#include <cstdint>       // uint8_t, uint16_t
#include <cerrno>        // errno
#include <vector>        // std::vector
#include <string>        // std::string

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsMessenger::DnsMessenger(int listenFd)
        : mListenFd{listenFd} {}

    vector<uint8_t> DnsMessenger::buildErrorReply(const uint8_t *pMessageBuffer,
                                                  const size_t messageLength,
                                                  const DnsRCodes rcode,
                                                  const DnsQuery &parsedQuery) {
        logger("DnsMessenger::buildErrorReply() called with buffer=%p, length=%zu, rcode=%d (%s)",
               static_cast<const void*>(pMessageBuffer), messageLength, CastUtils::castEnumToInt(rcode),
               CastUtils::castEnumToString<DnsRCodes>(rcode).c_str());
        verbose("Building DNS error response with code %d", CastUtils::castEnumToInt(rcode));

        // Check if the parsed query is valid (has valid qEndOffset)
        const bool isQueryValid = (parsedQuery.mQEndOffset != DnsQuery::EMPTY_FIELD);
        logger("Query validity check: isValid=%s, qEndOffset=%zu",
               isQueryValid ? "true" : "false", parsedQuery.mQEndOffset);

        // First we need to determine the end of the Question section
        size_t qEndOffset{0};
        if(isQueryValid) {
            qEndOffset = min(parsedQuery.mQEndOffset, messageLength);
            logger("Using parsed query offset: original=%zu, clamped=%zu",
                   parsedQuery.mQEndOffset, qEndOffset);
        }
        else {
            qEndOffset = getQuestionEndOffset(pMessageBuffer, messageLength);
            logger("Calculated question end offset from raw buffer: %zu", qEndOffset);
        }

        // The message must be at least as long as the DNS header
        if(qEndOffset < DnsQuery::HEADER_TRUE_SIZE) {
            logger("buildErrorReply failed: qEndOffset=%zu < required header size=%u",
                   qEndOffset, DnsQuery::HEADER_TRUE_SIZE);
            verbose("Cannot build DNS error response - message too short");
            return {};
        }
        logger("Question section validation passed: qEndOffset=%zu >= header_size=%u",
               qEndOffset, DnsQuery::HEADER_TRUE_SIZE);

        // We copy the original message buffer into a vector up to the end of the Question section
        vector<uint8_t> outMessage(qEndOffset);
        memcpy(outMessage.data(), pMessageBuffer, qEndOffset);
        logger("Copied %zu bytes from original message to response buffer", qEndOffset);

        // We set the DNS header flags (able to work even with invalid parsed query)
        uint16_t dnsRequestFlags{0};
        if(isQueryValid) {
            dnsRequestFlags = parsedQuery.mHeader.mFlags;
            logger("Using flags from parsed query: 0x%04X", dnsRequestFlags);
        }
        else {
            dnsRequestFlags = static_cast<uint16_t>(
                (pMessageBuffer[DnsHeader::FLAGS_MSB] << 8) | pMessageBuffer[DnsHeader::FLAGS_LSB]
            );
            logger("Extracted flags from raw buffer: bytes[%u][%u] = 0x%04X",
                   pMessageBuffer[DnsHeader::FLAGS_MSB], pMessageBuffer[DnsHeader::FLAGS_LSB],
                   dnsRequestFlags);
        }

        const uint16_t dnsReplyFlags = setResponseFlags(dnsRequestFlags, rcode);
        storeBigEndianWordToMessage(outMessage, DnsHeader::FLAGS_MSB, dnsReplyFlags);
        logger("Converted request flags 0x%04X to response flags 0x%04X with rcode=%d (%s)",
               dnsRequestFlags, dnsReplyFlags, CastUtils::castEnumToInt(rcode),
               CastUtils::castEnumToString<DnsRCodes>(rcode).c_str());

        // We set the counts in the DNS header
        logger("Setting DNS header counts: QDCOUNT=1, ANCOUNT=0, NSCOUNT=0, ARCOUNT=0");
        storeBigEndianWordToMessage(outMessage, DnsHeader::QDCOUNT_MSB, 1);  // We always copy one question
        storeBigEndianWordToMessage(outMessage, DnsHeader::ANCOUNT_MSB, 0);  // No answers
        storeBigEndianWordToMessage(outMessage, DnsHeader::NSCOUNT_MSB, 0);  // No authority records
        storeBigEndianWordToMessage(outMessage, DnsHeader::ARCOUNT_MSB, 0);  // No additional records

        logger("Successfully built error reply message: %zu bytes, rcode=%d (%s)",
               outMessage.size(), CastUtils::castEnumToInt(rcode),
               CastUtils::castEnumToString<DnsRCodes>(rcode).c_str());
        verbose("DNS error response ready for transmission (%zu bytes)", outMessage.size());
        return outMessage;  // Return the constructed error reply message
    } // DnsMessenger::buildErrorReply

    void DnsMessenger::sendDnsReply(const vector<uint8_t> &dnsReply, const sockaddr_in &destinationAddress) const {
        logger("DnsMessenger::sendDnsReply() called with message size=%zu, dest_addr=%s:%u",
               dnsReply.size(), inet_ntoa(destinationAddress.sin_addr), ntohs(destinationAddress.sin_port));
        verbose("Sending DNS response to client %s:%u (%zu bytes)",
                inet_ntoa(destinationAddress.sin_addr), ntohs(destinationAddress.sin_port), dnsReply.size());

        if(dnsReply.empty()) {
            logger("sendDnsReply failed: DNS reply message is empty");
            verbose("Cannot send empty DNS response");
            throw InternalErrorException(
                    "DNS reply message is empty eventhough it shuoldn't be. "
                    "Cannot send empty message."
                    );
        }
        logger("Message validation passed: %zu bytes ready for transmission", dnsReply.size());

        // Send the message
        logger("Calling sendto() with socket FD=%d, buffer size=%zu", mListenFd, dnsReply.size());
        const ssize_t bytesSent = sendto(mListenFd, dnsReply.data(), dnsReply.size(), 0,
                                         reinterpret_cast<const sockaddr*>(&destinationAddress),
                                         sizeof(destinationAddress));

        // If the sendto() function returns an error
        if(bytesSent < 0) {
            logger("sendto() failed: FD=%d, errno=%d (%s), attempted_bytes=%zu",
                   mListenFd, errno, strerror(errno), dnsReply.size());
            verbose("Failed to send DNS response to %s:%u - network error",
                    inet_ntoa(destinationAddress.sin_addr), ntohs(destinationAddress.sin_port));
            throw ConnectionErrorException(
                    "Failed to send DNS reply to the client due to sendto() error: " + string(strerror(errno))
                    );
        }
        // If the sendto() function returns 0, it means the connection has been closed
        if(bytesSent == 0) {
            logger("sendto() returned 0: connection appears closed, FD=%d", mListenFd);
            verbose("Connection closed - cannot send DNS response");
            throw ConnectionErrorException(
                    "Connection closed by client. Unable to send DNS reply to the client."
                    );
        }

        logger("sendto() successful: sent %zd/%zu bytes to %s:%u via FD=%d",
               bytesSent, dnsReply.size(), inet_ntoa(destinationAddress.sin_addr),
               ntohs(destinationAddress.sin_port), mListenFd);
        verbose("DNS response sent successfully to %s:%u (%zd bytes)",
                inet_ntoa(destinationAddress.sin_addr), ntohs(destinationAddress.sin_port), bytesSent);
    } // DnsMessenger::sendDnsReply

    void DnsMessenger::sendRefusedMessage(const uint8_t *pMessageBuffer,
                                          const size_t messageLength,
                                          const sockaddr_in &clientAddress,
                                          const DnsQuery &dnsQuery) const {
        logger("DnsMessenger::sendRefusedMessage() called for client %s:%u",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        verbose("Sending REFUSED response to %s:%u - query rejected",
                inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::REFUSED, dnsQuery);
        sendDnsReply(message, clientAddress);

        logger("REFUSED message sent successfully");
    } // DnsMessenger::sendRefusedMessage

    void DnsMessenger::sendNotImpMessage(const uint8_t *pMessageBuffer,
                                         const size_t messageLength,
                                         const sockaddr_in &clientAddress) const {
        logger("DnsMessenger::sendNotImpMessage() called for client %s:%u",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        verbose("Sending NOT IMPLEMENTED response to %s:%u - unsupported operation",
                inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::NOTIMP);
        sendDnsReply(message, clientAddress);

        logger("NOTIMP message sent successfully");
    } // DnsMessenger::sendNotImpMessage

    void DnsMessenger::sendFormErrMessage(const uint8_t *pMessageBuffer,
                                          const size_t messageLength,
                                          const sockaddr_in &clientAddress) const {
        logger("DnsMessenger::sendFormErrMessage() called for client %s:%u",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        verbose("Sending FORMAT ERROR response to %s:%u - malformed query",
                inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::FORMERR);
        sendDnsReply(message, clientAddress);

        logger("FORMERR message sent successfully");
    } // DnsMessenger::sendFormErrMessage

    void DnsMessenger::sendServFailMessage(const uint8_t *pMessageBuffer,
                                           const size_t messageLength,
                                           const sockaddr_in &clientAddress) const {
        logger("DnsMessenger::sendServFailMessage() called for client %s:%u",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        verbose("Sending SERVER FAILURE response to %s:%u - internal error",
                inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::SERVFAIL);
        sendDnsReply(message, clientAddress);

        logger("SERVFAIL message sent successfully");
    } // DnsMessenger::sendServFailMessage

    void DnsMessenger::storeBigEndianWordToMessage(vector<uint8_t> &messageBuffer,
                                                   const size_t msbIndex,
                                                   const uint16_t valueToStore) {
        logger("DnsMessenger::storeBigEndianWordToMessage() called: index=%zu, value=0x%04X (%u)",
               msbIndex, valueToStore, valueToStore);

        // Check bounds
        if(msbIndex + 1 >= messageBuffer.size()) {
            logger("storeBigEndianWordToMessage failed: index %zu+1 >= buffer_size %zu",
                   msbIndex, messageBuffer.size());
            return;
        }
        logger("Bounds check passed: %zu+1 < %zu", msbIndex, messageBuffer.size());

        // Store in network byte order (big-endian)
        const auto bytes = CastUtils::castWordToTwoBytes<uint16_t>(valueToStore);
        messageBuffer[msbIndex] = bytes[0];
        messageBuffer[msbIndex + 1] = bytes[1];

        logger("Stored value 0x%04X as bytes [%u][%u] at indices [%zu][%zu]",
               valueToStore, bytes[0], bytes[1], msbIndex, msbIndex + 1);
    } // DnsMessenger::storeBigEndianWordToMessage

    uint16_t DnsMessenger::setResponseFlags(const uint16_t flagsFromRequest, DnsRCodes rcode) {
        logger("DnsMessenger::setResponseFlags() called: input_flags=0x%04X, rcode=%d (%s)",
               flagsFromRequest, CastUtils::castEnumToInt(rcode),
               CastUtils::castEnumToString<DnsRCodes>(rcode).c_str());

        // We start with the original flags
        uint16_t setFlags = flagsFromRequest;
        logger("Starting with original flags: 0x%04X", setFlags);

        // QR = 1 (response)
        setFlags |= DnsHeader::QR_MASK;
        logger("Set QR bit (response): 0x%04X", setFlags);

        // AA = 0 (this isn't an authoritative answer)
        setFlags &= static_cast<uint16_t>(~DnsHeader::AA_MASK);
        logger("Cleared AA bit (non-authoritative): 0x%04X", setFlags);

        // Z bit must be 0 (6th bit)
        setFlags &= static_cast<uint16_t>(~DnsHeader::Z_MASK);
        logger("Cleared Z bit (reserved): 0x%04X", setFlags);

        // RCODE
        setFlags &= static_cast<uint16_t>(~DnsHeader::RCODE_MASK);
        setFlags |= static_cast<uint16_t>(rcode) & DnsHeader::RCODE_MASK;
        logger("Set RCODE to %d (%s): final_flags=0x%04X", CastUtils::castEnumToInt(rcode),
               CastUtils::castEnumToString<DnsRCodes>(rcode).c_str(), setFlags);

        return setFlags;
    } // DnsMessenger::setResponseFlags

    size_t DnsMessenger::getQuestionEndOffset(const uint8_t *messageBuffer, const size_t messageLength) {
        logger("DnsMessenger::getQuestionEndOffset() called with buffer=%p, length=%zu",
               static_cast<const void*>(messageBuffer), messageLength);

        // Minimal length represents only DNS header
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("getQuestionEndOffset failed: message too short (%zu < %u)",
                   messageLength, DnsQuery::HEADER_TRUE_SIZE);
            return INVALID_MESSAGE_LENGTH;
        }
        logger("Message length validation passed: %zu >= %u", messageLength, DnsQuery::HEADER_TRUE_SIZE);

        // Beginning offset it the header length
        size_t offset = DnsQuery::HEADER_TRUE_SIZE;
        logger("Starting QNAME parsing at offset %zu (after header)", offset);

        // We must have at least one question
        if(offset >= messageLength) {
            logger("getQuestionEndOffset failed: no space for question section (%zu >= %zu)",
                   offset, messageLength);
            return INVALID_MESSAGE_LENGTH;
        }

        // Now we must parse the QNAME of the question
        size_t labelCount = 0;
        do {
            // Each label starts with a length byte
            const uint8_t labelLength = messageBuffer[offset++];
            logger("Read label %zu length: %u at offset %zu", labelCount, labelLength, offset - 1);

            // A zero length indicates the end of the QNAME (root label)
            if(labelLength == 0) {
                logger("Found QNAME terminator (0-length label) at offset %zu", offset - 1);
                break;
            }

            // Check for pointer (we do not support pointers in QNAME)
            if((labelLength & DNS_LABEL_POINTER_MASK) != 0) {
                logger("getQuestionEndOffset failed: compression pointer detected (0x%02X)",
                       labelLength);
                return INVALID_MESSAGE_LENGTH;
            }

            // Move the offset forward by the length of the label
            if(offset + labelLength > messageLength) {
                logger("getQuestionEndOffset failed: label extends beyond buffer (%zu + %u > %zu)",
                       offset, labelLength, messageLength);
                return INVALID_MESSAGE_LENGTH;
            }

            logger("Processing label %zu: length=%u, content at offset %zu", labelCount, labelLength, offset);

            offset += labelLength;  // move offset past processed label
            labelCount++;

            logger("Advanced offset to %zu after processing label %zu", offset, labelCount - 1);
        } while(offset < messageLength);

        // After QNAME, we must have QTYPE (2 bytes) and QCLASS (2 bytes)
        offset += DnsQuery::QTYPE_SIZE + DnsQuery::QCLASS_SIZE;
        logger("Adding QTYPE+QCLASS size (2+2 bytes): final offset=%zu", offset);

        if(offset > messageLength) {
            logger("getQuestionEndOffset failed: QTYPE/QCLASS extends beyond buffer (%zu > %zu)",
                   offset, messageLength);
            return INVALID_MESSAGE_LENGTH;
        }

        logger("Successfully calculated question end offset: %zu (processed %zu labels)",
               offset, labelCount);
        return offset;  // valid length of Header + Question
    } // DnsMessenger::getQuestionEndOffset
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsMessenger.cpp ***/
