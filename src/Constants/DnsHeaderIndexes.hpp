/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeaderIndexes.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsHeaderIndexes` class, which     *
 *               defines byte offsets for DNS header fields according to       *
 *               RFC 1035. These constants are used for direct byte-level      *
 *               access to DNS message header components.                      *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeaderIndexes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsHeaderIndexes` class containing byte offset
 *        constants for DNS header fields.
 */

#ifndef DNS_HEADER_INDEXES_HPP
#define DNS_HEADER_INDEXES_HPP

namespace FilteringDnsResolver::Constants
{
    /**
     * @class DnsHeaderIndexes
     * @brief Provides byte offset constants for DNS header fields.
     *
     * @details This class contains static constants representing byte offsets
     *          for accessing individual fields within the 12-byte DNS header
     *          structure. Each 16-bit field is split into MSB (Most Significant
     *          Byte) and LSB (Least Significant Byte) for direct byte-level
     *          manipulation of DNS messages according to RFC 1035.
     *
     * @note All offsets assume network byte order (big-endian) where MSB
     *       comes before LSB in the byte stream.
     */
    class DnsHeaderIndexes {
    public:
        static constexpr auto ID_MSB{0};        /**< Byte 0: Transaction ID most significant byte             */
        static constexpr auto ID_LSB{1};        /**< Byte 1: Transaction ID least significant byte            */
        static constexpr auto FLAGS_MSB{2};     /**< Byte 2: Flags field most significant byte                */
        static constexpr auto FLAGS_LSB{3};     /**< Byte 3: Flags field least significant byte               */
        static constexpr auto QDCOUNT_MSB{4};   /**< Byte 4: Question count most significant byte             */
        static constexpr auto QDCOUNT_LSB{5};   /**< Byte 5: Question count least significant byte            */
        static constexpr auto ANCOUNT_MSB{6};   /**< Byte 6: Answer count most significant byte               */
        static constexpr auto ANCOUNT_LSB{7};   /**< Byte 7: Answer count least significant byte              */
        static constexpr auto NSCOUNT_MSB{8};   /**< Byte 8: Authority records count most significant byte    */
        static constexpr auto NSCOUNT_LSB{9};   /**< Byte 9: Authority records count least significant byte   */
        static constexpr auto ARCOUNT_MSB{10};  /**< Byte 10: Additional records count most significant byte  */
        static constexpr auto ARCOUNT_LSB{11};  /**< Byte 11: Additional records count least significant byte */
    }; // DnsHeaderIndexes
} // FilteringDnsResolver::Constants

#endif // DNS_HEADER_INDEXES_HPP

/*** end of file DnsHeaderIndexes.hpp ***/
