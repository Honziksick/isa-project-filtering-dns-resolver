/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeader.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsHeader` class, which            *
 *               represents and manipulates DNS message headers according to   *
 *               RFC 1035. It provides methods for extracting and checking     *
 *               individual flag bits and fields within the DNS header         *
 *               structure.                                                    *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeader.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsHeader` class for DNS message header
 *        representation and manipulation.
 *
 * @note This implementation was inspired by (primarily section 2):
 *       `https://medium.com/@s12deff/command-and-control-c2-dns-server-part-i-d662a6764aff`
 */

#ifndef DNS_HEADER_HPP
#define DNS_HEADER_HPP

#include "Constants/DnsHeaderFlagMasks.hpp"
#include "Constants/DnsHeaderIndexes.hpp"
#include "Enums/DnsRCodes.hpp"
#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class DnsHeader
     * @brief Represents and manipulates DNS message headers according to RFC 1035.
     *
     * @details This class provides functionality for working with DNS message headers,
     *          including storage of header fields and utility methods for extracting
     *          and checking individual flag bits. It inherits constants for byte
     *          offsets and bit masks from the corresponding constants classes to
     *          provide a complete interface for DNS header manipulation.
     *
     * @note The class uses network byte order (big-endian) for all multi-byte
     *       fields as required by the DNS protocol specification.
     */
    class DnsHeader : public Constants::DnsHeaderIndexes, public Constants::DnsHeaderFlagMasks {
    public:
        /**
         * @brief Default constructor creating an empty DNS header.
         */
        DnsHeader() = default;

        /**
         * @brief Constructs a DNS header with specified basic fields.
         *
         * @details Creates a DNS header with the essential fields for DNS queries.
         *          Other fields (answer count, authority count, additional count)
         *          are initialized to zero as appropriate for query messages.
         *
         * @param id Transaction identifier for matching queries and responses.
         * @param flags 16-bit flags field containing QR, OPCODE, AA, TC, RD, RA, Z, AD, CD, and RCODE bits.
         * @param qdCount Number of questions in the question section.
         */
        explicit DnsHeader(uint16_t id, uint16_t flags, uint16_t qdCount);

        uint16_t mId{EMPTY_FIELD};       /**< Transaction ID for matching queries and responses.      */
        uint16_t mFlags{EMPTY_FIELD};    /**< Flags field containing various control and status bits. */
        uint16_t mQdCount{EMPTY_FIELD};  /**< Number of entries in the question section.              */

        static constexpr uint16_t EMPTY_FIELD{0}; /**< Constant representing an uninitialized field value. */

        /**
         * @brief Checks if the QR (Query/Response) bit is set in the flags field.
         *
         * @details The QR bit indicates whether the message is a query (0) or
         *          a response (1). This is the most significant bit in the flags field.
         *
         * @param flags The 16-bit flags field to examine.
         *
         * @return `true` if the message is a response, `false` if it's a query.
         */
        [[nodiscard]]
        static bool isQRSet(uint16_t flags);

        /**
         * @brief Checks if the RD (Recursion Desired) bit is set in the flags field.
         *
         * @details The RD bit indicates whether the client desires recursive
         *          query processing by the DNS server. This bit is typically
         *          set in queries from stub resolvers.
         *
         * @param flags The 16-bit flags field to examine.
         *
         * @return `true` if recursion is desired, `false` otherwise.
         */
        [[nodiscard]]
        static bool isRDSet(uint16_t flags);

        /**
         * @brief Extracts the OPCODE field from the flags.
         *
         * @details The OPCODE field (bits 14-11) specifies the type of query.
         *          Standard values include 0 (QUERY), 1 (IQUERY), and 2 (STATUS).
         *          Most DNS messages use OPCODE 0 for standard queries.
         *
         * @param flags The 16-bit flags field to examine.
         *
         * @return The 4-bit OPCODE value (0-15).
         */
        [[nodiscard]]
        static uint16_t getOpcode(uint16_t flags);

        /**
         * @brief Checks if the Z (reserved) bit is properly set to zero.
         *
         * @details The Z bit (bit 6) is reserved for future use and must be
         *          set to zero in all current DNS implementations. This method
         *          can be used to validate message compliance.
         *
         * @param flags The 16-bit flags field to examine.
         *
         * @return `true` if the Z bit is correctly set to 0,
         *         `false` if it's improperly set to 1.
         */
        [[nodiscard]]
        static bool isZBitZero(uint16_t flags);

    private:
        using enum Enums::DnsRCodes;
        static constexpr int OPCODE_SHIFT{11};  /**< Bit shift amount for extracting OPCODE from flags field. */
    }; // DnsHeader
} // FilteringDnsResolver::DNS

#endif // DNS_HEADER_HPP

/*** end of file DnsHeader.hpp ***/
