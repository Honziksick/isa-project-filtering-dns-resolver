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
 * Description:  This header file provides `DnsMessageParser` class, which     *
 *               implements parsing and validation of DNS query messages       *
 *               according to RFC 1035. It provides functionality for          *
 *               extracting DNS header fields, QNAME domain names, QTYPE and   *
 *               QCLASS values from raw message buffers with comprehensive     *
 *               validation of message format and protocol compliance.         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessageParser.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsMessageParser` class for DNS message
 *        parsing and validation functionality.
 */

#ifndef DNS_MESSAGE_PARSER_HPP
#define DNS_MESSAGE_PARSER_HPP

#include "DnsUtils/DnsHeader.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include <cstdint>  // uint8_t, uint16_t
#include <string>   // std::string

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class DnsMessageParser
     * @brief Parses and validates DNS query messages according to RFC 1035.
     *
     * @details Provides static methods for extracting DNS header, QNAME, QTYPE,
     *          and QCLASS from raw message buffers. Supports only standard DNS
     *          queries (OPCODE 0) with A records (QTYPE=1) and IN class (QCLASS=1).
     *          DNS message compression is not supported.
     */
    class DnsMessageParser {
    public:
        /**
         * @brief Parses and validates a complete DNS query message.
         *
         * @details Extracts DNS header, QNAME domain name, QTYPE and QCLASS fields
         *          from raw byte buffer. Validates all components for RFC 1035
         *          compliance. QNAME is converted to lowercase for case-insensitive
         *          processing.
         *
         * @param pMessageBuffer Pointer to the raw DNS message buffer.
         * @param messageBufferLength Length of the message buffer in bytes.
         * @param outQuery Reference to `DnsQuery` object to populate with parsed data.
         *
         * @throws DnsParseErrorException If message is malformed, incomplete, or
         *                                violates DNS protocol rules with specific
         *                                RCODE and component information.
         */
        static void parseAndValidate(const uint8_t *pMessageBuffer, size_t messageBufferLength, DnsQuery &outQuery);

    private:
        static constexpr size_t MAX_POINTER_CHAIN{128}; /**< Max pointer jumps to prevent loops */

        /**
         * @brief Parses the DNS header from the message buffer.
         *
         * @details Extracts 12-byte DNS header fields (ID, FLAGS, QDCOUNT) and
         *          converts from network byte order to host byte order.
         *
         * @param pMessageBuffer Pointer to the raw DNS message buffer.
         * @param messageBufferLength Length of the message buffer in bytes.
         * @param outDnsHeader Reference to the DnsHeader object to populate.
         *
         * @return `true` if header parsed successfully, `false` if message too short.
         */
        static bool parseHeader(const uint8_t *pMessageBuffer, size_t messageBufferLength,
                                DnsHeader &outDnsHeader);

        /**
         * @brief Parses the QNAME (domain name) from the message buffer.
         *
         * @details Parses DNS label sequence with length-prefixed labels ending
         *          with zero-length label. Converts to lowercase and stores with
         *          dot separators. DNS compression is not supported.
         *
         * @param pMessageBuffer Pointer to the raw DNS message buffer.
         * @param messageBufferLength Length of the message buffer in bytes.
         * @param inOutOffset Reference to current parsing offset (updated).
         * @param outQName Reference to string for storing parsed domain name.
         *
         * @return `true` if QNAME parsed successfully, `false` if invalid format.
         */
        static bool parseQName(const uint8_t *pMessageBuffer, size_t messageBufferLength,
                               size_t &inOutOffset, std::string &outQName);

        /**
         * @brief Parses the QTYPE field from the message buffer.
         *
         * @details Extracts 16-bit QTYPE field specifying DNS record type
         *          and converts from network byte order to host byte order.
         *
         * @param pMessageBuffer Pointer to the raw DNS message buffer.
         * @param messageBufferLength Length of the message buffer in bytes.
         * @param currentOffset Reference to current parsing offset (updated by 2 bytes).
         *
         * @return The QTYPE value in host byte order.
         *
         * @throws DnsParseErrorException If buffer too short for QTYPE field.
         */
        static uint16_t parseQType(const uint8_t *pMessageBuffer, size_t messageBufferLength,
                                   size_t &currentOffset);

        /**
         * @brief Parses the QCLASS field from the message buffer.
         *
         * @details Extracts 16-bit QCLASS field specifying DNS query class
         *          and converts from network byte order to host byte order.
         *
         * @param pMessageBuffer Pointer to the raw DNS message buffer.
         * @param messageBufferLength Length of the message buffer in bytes.
         * @param currentOffset Reference to current parsing offset (updated by 2 bytes).
         *
         * @return The QCLASS value in host byte order.
         *
         * @throws DnsParseErrorException If buffer too short for QCLASS field.
         */
        static uint16_t parseQClass(const uint8_t *pMessageBuffer, size_t messageBufferLength,
                                    size_t &currentOffset);

        /**
         * @brief Validates parsed DNS query components for protocol compliance.
         *
         * @details Validates header flags, question count, and query type/class.
         *          Only supports QTYPE=1 (A records), QCLASS=1 (IN), OPCODE=0
         *          (standard query). QR bit must be 0, Z bit must be 0, QDCOUNT must be 1.
         *
         * @param dnsHeader The parsed DNS header to validate.
         * @param qtype The parsed QTYPE value to validate.
         * @param qclass The parsed QCLASS value to validate.
         *
         * @throws DnsParseErrorException If validation fails with specific RCODE
         *                                and component information.
         */
        static void validateDnsQuery(const DnsHeader &dnsHeader, uint16_t qtype, uint16_t qclass);
    }; // DnsMessageParser
} // FilteringDnsResolver::DnsUtils

#endif // DNS_MESSAGE_PARSER_HPP

/*** end of file DnsMessageParser.hpp ***/
