/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsMessenger.hpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    07.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsMessenger` class, which         *
 *               handles sending DNS responses and error messages to clients.  *
 *               It provides functionality for constructing and sending        *
 *               various DNS response types including successful replies       *
 *               and error responses with appropriate RCODE values according   *
 *               to RFC 1035.                                                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessenger.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsMessenger` class for DNS response
 *        construction and transmission functionality.
 */

#ifndef DNS_MESSENGER_HPP
#define DNS_MESSENGER_HPP

#include "DnsUtils/DnsQuery.hpp"
#include "Enums/DnsRCodes.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <cstdint>       // uint8_t, uint16_t
#include <vector>        // std::vector

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class DnsMessenger
     * @brief Handles sending DNS responses and error messages to clients.
     *
     * @details Provides functionality for constructing and transmitting various
     *          types of DNS responses including successful replies and error
     *          responses with appropriate RCODE values. Manages response flag
     *          setting and message formatting according to RFC 1035.
     */
    class DnsMessenger {
    public:
        /**
         * @brief Constructs a DnsMessenger with the specified listening socket.
         *
         * @details Initializes the messenger with a UDP socket file descriptor
         *          for sending DNS responses to clients.
         *
         * @param listenFd File descriptor of the UDP socket for client communication.
         */
        explicit DnsMessenger(int listenFd);

        /**
         * @brief Sends a complete DNS reply to the specified client address.
         *
         * @details Transmits a pre-constructed DNS response message to the client
         *          using the configured UDP socket.
         *
         * @param dnsReply Complete DNS response message as byte vector.
         * @param destinationAddress Client address to send the response to.
         *
         * @throws ConnectionErrorException If sending fails.
         */
        void sendDnsReply(const std::vector<uint8_t> &dnsReply, const sockaddr_in &destinationAddress) const;

        /**
         * @brief Sends a REFUSED error response to the client.
         *
         * @details Constructs and sends a DNS response with RCODE=5 (REFUSED)
         *          indicating the server refuses to perform the requested operation.
         *          Used when queries are blocked by filtering rules.
         *
         * @param pMessageBuffer Original query message buffer.
         * @param messageLength Length of the original message.
         * @param clientAddress Client address to send the error response to.
         * @param dnsQuery Parsed query object for response construction.
         */
        void sendRefusedMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                                const sockaddr_in &clientAddress, const DnsQuery &dnsQuery) const;

        /**
         * @brief Sends a NOTIMP error response to the client.
         *
         * @details Constructs and sends a DNS response with RCODE=4 (NOTIMP)
         *          indicating the requested operation is not implemented.
         *          Used for unsupported query types or operations.
         *
         * @param pMessageBuffer Original query message buffer.
         * @param messageLength Length of the original message.
         * @param clientAddress Client address to send the error response to.
         */
        void sendNotImpMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                               const sockaddr_in &clientAddress) const;

        /**
         * @brief Sends a FORMERR error response to the client.
         *
         * @details Constructs and sends a DNS response with RCODE=1 (FORMERR)
         *          indicating the query message format is invalid or malformed.
         *
         * @param pMessageBuffer Original query message buffer.
         * @param messageLength Length of the original message.
         * @param clientAddress Client address to send the error response to.
         */
        void sendFormErrMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                                const sockaddr_in &clientAddress) const;

        /**
         * @brief Sends a SERVFAIL error response to the client.
         *
         * @details Constructs and sends a DNS response with RCODE=2 (SERVFAIL)
         *          indicating a server failure occurred while processing the query.
         *
         * @param pMessageBuffer Original query message buffer.
         * @param messageLength Length of the original message.
         * @param clientAddress Client address to send the error response to.
         */
        void sendServFailMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                                 const sockaddr_in &clientAddress) const;

    private:
        int mListenFd;  /**< File descriptor of the UDP socket for client communication */

        static constexpr auto INVALID_MESSAGE_LENGTH{0};        /**< Constant indicating invalid message length */
        static constexpr uint8_t DNS_LABEL_POINTER_MASK{0xC0};  /**< Bitmask for detecting DNS label compression pointers */

        /**
         * @brief Constructs a DNS error response message.
         *
         * @details Builds a complete DNS error response with the specified RCODE,
         *          preserving the original query's transaction ID and question
         *          section while setting appropriate response flags.
         *
         * @param pMessageBuffer Original query message buffer.
         * @param messageLength Length of the original message.
         * @param rcode DNS response code to set in the response.
         * @param parsedQuery Parsed query object (optional, for REFUSED responses).
         *
         * @return Complete DNS error response as byte vector.
         */
        static std::vector<uint8_t> buildErrorReply(const uint8_t *pMessageBuffer,
                                                    size_t messageLength,
                                                    Enums::DnsRCodes rcode,
                                                    const DnsQuery &parsedQuery = DnsQuery{});

        /**
         * @brief Stores a 16-bit value in big-endian format to message buffer.
         *
         * @details Converts a 16-bit value to network byte order and stores it
         *          at the specified position in the message buffer.
         *
         * @param messageBuffer Message buffer to modify.
         * @param msbIndex Index where the most significant byte should be stored.
         * @param valueToStore 16-bit value to store in big-endian format.
         */
        static void storeBigEndianWordToMessage(std::vector<uint8_t> &messageBuffer,
                                                size_t msbIndex,
                                                uint16_t valueToStore);

        /**
         * @brief Sets appropriate response flags for the given RCODE.
         *
         * @details Modifies the flags field from the original query to create
         *          appropriate response flags, setting QR=1, clearing TC and setting
         *          the specified RCODE value.
         *
         * @param flagsFromRequest Original flags field from the query.
         * @param rcode DNS response code to set in the flags.
         *
         * @return Modified flags field for the response.
         */
        static uint16_t setResponseFlags(uint16_t flagsFromRequest, Enums::DnsRCodes rcode);

        /**
         * @brief Finds the end offset of the question section in a DNS message.
         *
         * @details Parses through the QNAME field to find where the question
         *          section ends, accounting for label compression if present.
         *
         * @param messageBuffer DNS message buffer to parse.
         * @param messageLength Length of the message buffer.
         *
         * @return Offset where the question section ends, or INVALID_MESSAGE_LENGTH if parsing fails.
         */
        static size_t getQuestionEndOffset(const uint8_t *messageBuffer, size_t messageLength);
    }; // DnsMessenger
} // FilteringDnsResolver::DnsUtils

#endif // DNS_MESSENGER_HPP

/*** end of file DnsMessenger.hpp ***/
