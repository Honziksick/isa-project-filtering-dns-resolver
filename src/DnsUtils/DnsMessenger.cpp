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
        // Check if the parsed query is valid (has valid qEndOffset)
        const bool isQueryValid = (parsedQuery.mQEndOffset != DnsQuery::EMPTY_FIELD);

        // First we need to determine the end of the Question section
        size_t qEndOffset{0};
        if(isQueryValid) {
            qEndOffset = min(parsedQuery.mQEndOffset, messageLength);
        }
        else {
            qEndOffset = getQuestionEndOffset(pMessageBuffer, messageLength);
        }

        // The message must be at least as long as the DNS header
        if(qEndOffset < DnsQuery::HEADER_TRUE_SIZE) {
            return {};
        }

        // We copy the original message buffer into a vector up to the end of the Question section
        vector<uint8_t> outMessage(qEndOffset);
        memcpy(outMessage.data(), pMessageBuffer, qEndOffset);

        // We set the DNS header flags (able to work even with invalid parsed query)
        uint16_t dnsRequestFlags{0};
        if(isQueryValid) {
            dnsRequestFlags = parsedQuery.mHeader.mFlags;
        }
        else {
            dnsRequestFlags = static_cast<uint16_t>(
                (pMessageBuffer[DnsHeader::FLAGS_MSB] << 8) | pMessageBuffer[DnsHeader::FLAGS_LSB]
            );
        }
        const uint16_t dnsReplyFlags = setResponseFlags(dnsRequestFlags, rcode);
        storeBigEndianWordToMessage(outMessage, DnsHeader::FLAGS_MSB, dnsReplyFlags);

        // We set the counts in the DNS header
        storeBigEndianWordToMessage(outMessage, DnsHeader::QDCOUNT_MSB, 1);  // We always copy one question
        storeBigEndianWordToMessage(outMessage, DnsHeader::ANCOUNT_MSB, 0);  // No answers
        storeBigEndianWordToMessage(outMessage, DnsHeader::NSCOUNT_MSB, 0);  // No authority records
        storeBigEndianWordToMessage(outMessage, DnsHeader::ARCOUNT_MSB, 0);  // No additional records

        return outMessage;  // Return the constructed error reply message
    } // DnsMessenger::buildErrorReply

    void DnsMessenger::sendDnsReply(const vector<uint8_t> &dnsReply, const sockaddr_in &destinationAddress) const {
        if(dnsReply.empty()) {
            throw InternalErrorException(
                    "DNS reply message is empty eventhough it shuoldn't be. "
                    "Cannot send empty message."
                    );
        }

        // Send the message
        const ssize_t bytesSent = sendto(mListenFd, dnsReply.data(), dnsReply.size(), 0,
                                         reinterpret_cast<const sockaddr*>(&destinationAddress),
                                         sizeof(destinationAddress));

        // If the sendto() function returns an error
        if(bytesSent < 0) {
            logger("sendto() returned error: Failed to send message. Socket 'FD = %d', "
                   "error: %s, Bytes: %zd", mListenFd, strerror(errno), bytesSent);
            throw ConnectionErrorException(
                    "Failed to send DNS reply to the client due to sendto() error: " + string(strerror(errno))
                    );
        }
        // If the sendto() function returns 0, it means the connection has been closed
        if(bytesSent == 0) {
            logger("sendto() returned 0: Connection closed by server");
            throw ConnectionErrorException(
                    "Connection closed by client. Unable to send DNS reply to the client."
                    );
        }
    } // DnsMessenger::sendDnsReply

    void DnsMessenger::sendRefusedMessage(const uint8_t *pMessageBuffer,
                                          const size_t messageLength,
                                          const sockaddr_in &clientAddress,
                                          const DnsQuery &dnsQuery) const {
        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::REFUSED, dnsQuery);
        sendDnsReply(message, clientAddress);
    } // DnsMessenger::sendRefusedMessage

    void DnsMessenger::sendNotImpMessage(const uint8_t *pMessageBuffer,
                                         const size_t messageLength,
                                         const sockaddr_in &clientAddress) const {
        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::NOTIMP);
        sendDnsReply(message, clientAddress);
    } // DnsMessenger::sendNotImpMessage

    void DnsMessenger::sendFormErrMessage(const uint8_t *pMessageBuffer,
                                          const size_t messageLength,
                                          const sockaddr_in &clientAddress) const {
        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::FORMERR);
        sendDnsReply(message, clientAddress);
    } // DnsMessenger::sendFormErrMessage

    void DnsMessenger::sendServFailMessage(const uint8_t *pMessageBuffer,
                                           const size_t messageLength,
                                           const sockaddr_in &clientAddress) const {
        const auto message = buildErrorReply(pMessageBuffer, messageLength, DnsRCodes::SERVFAIL);
        sendDnsReply(message, clientAddress);
    } // DnsMessenger::sendServFailMessage

    void DnsMessenger::storeBigEndianWordToMessage(vector<uint8_t> &messageBuffer,
                                                   const size_t msbIndex,
                                                   const uint16_t valueToStore) {
        // Check bounds
        if(msbIndex + 1 >= messageBuffer.size()) {
            return;
        }

        // Store in network byte order (big-endian)
        const auto bytes = CastUtils::castWordToTwoBytes<uint16_t>(valueToStore);
        messageBuffer[msbIndex] = bytes[0];
        messageBuffer[msbIndex + 1] = bytes[1];
    } // DnsMessenger::storeBigEndianWordToMessage

    uint16_t DnsMessenger::setResponseFlags(const uint16_t flagsFromRequest, DnsRCodes rcode) {
        // We start with the original flags
        uint16_t setFlags = flagsFromRequest;

        // QR = 1 (response)
        setFlags |= DnsHeader::QR_MASK;

        // AA = 0 (this isn't an authoritative answer)
        setFlags &= static_cast<uint16_t>(~DnsHeader::AA_MASK);

        // Z bit must be 0 (6th bit)
        setFlags &= static_cast<uint16_t>(~DnsHeader::Z_MASK);

        // RCODE
        setFlags &= static_cast<uint16_t>(~DnsHeader::RCODE_MASK);
        setFlags |= static_cast<uint16_t>(rcode) & DnsHeader::RCODE_MASK;

        return setFlags;
    } // DnsMessenger::setResponseFlags

    size_t DnsMessenger::getQuestionEndOffset(const uint8_t *messageBuffer, const size_t messageLength) {
        // Minimal length represents only DNS header
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            return INVALID_MESSAGE_LENGTH;
        }

        // Beginning offset it the header length
        size_t offset = DnsQuery::HEADER_TRUE_SIZE;

        // We must have at least one question
        if(offset >= messageLength) {
            return INVALID_MESSAGE_LENGTH;
        }

        // Now we must parse the QNAME of the question
        do {
            // Each label starts with a length byte
            const uint8_t labelLength = messageBuffer[offset++];

            // A zero length indicates the end of the QNAME (root label)
            if(labelLength == 0) {
                break;
            }

            // Check for pointer (we do not support pointers in QNAME)
            if((labelLength & DNS_LABEL_POINTER_MASK) != 0) {
                return INVALID_MESSAGE_LENGTH;
            }

            // Move the offset forward by the length of the label
            if(offset + labelLength > messageLength) {
                return INVALID_MESSAGE_LENGTH;
            }

            offset += labelLength;  // move offset past processed label
        } while(offset < messageLength);

        // After QNAME, we must have QTYPE (2 bytes) and QCLASS (2 bytes)
        offset += DnsQuery::QTYPE_SIZE + DnsQuery::QCLASS_SIZE;
        if(offset > messageLength) {
            return INVALID_MESSAGE_LENGTH;
        }

        return offset;  // valid length of Header + Question
    } // DnsMessenger::getQuestionEndOffset
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsMessenger.cpp ***/
