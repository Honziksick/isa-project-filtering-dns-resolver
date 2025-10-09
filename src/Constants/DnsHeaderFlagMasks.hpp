/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeaderFlagMasks.hpp                                        *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsHeaderFlagMasks` class, which   *
 *               defines bit masks for DNS header flags according to RFC 1035  *
 *               and related RFCs. These masks are used for extracting and     *
 *               manipulating individual flag bits in DNS message headers.     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeaderFlagMasks.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsHeaderFlagMasks` class containing bit masks
 *        for DNS header flags manipulation.
 */

#ifndef DNS_HEADER_FLAG_MASKS_HPP
#define DNS_HEADER_FLAG_MASKS_HPP

#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Constants
{
    /**
     * @class DnsHeaderFlagMasks
     * @brief Provides bit masks for DNS header flags manipulation.
     *
     * @details This class contains static constants representing bit masks
     *          for individual fields within the DNS header flags word.
     *          These masks follow RFC 1035 specification and can be used
     *          with bitwise operations to extract or set specific flags
     *          in DNS message headers.
     *
     * @note All masks are designed for use with the 16-bit flags field
     *       in network byte order.
     */
    class DnsHeaderFlagMasks {
    public:
        static constexpr uint16_t QR_MASK{0x8000};      /**< Bit 15: QR (Query=0 / Response=1)        */
        static constexpr uint16_t OPCODE_MASK{0x7800};  /**< Bits 14-11: OPCODE (usually 0=QUERY)     */
        static constexpr uint16_t AA_MASK{0x0400};      /**< Bit 10: AA (Authoritative Answer)        */
        static constexpr uint16_t TC_MASK{0x0200};      /**< Bit 9: TC (Truncated)                    */
        static constexpr uint16_t RD_MASK{0x0100};      /**< Bit 8: RD (Recursion Desired)            */
        static constexpr uint16_t RA_MASK{0x0080};      /**< Bit 7: RA (Recursion Available)          */
        static constexpr uint16_t Z_MASK{0x0040};       /**< Bit 6: Z (reserved, must be 0)           */
        static constexpr uint16_t AD_MASK{0x0020};      /**< Bit 5: AD (Authenticated Data, RFC 4035) */
        static constexpr uint16_t CD_MASK{0x0010};      /**< Bit 4: CD (Checking Disabled, RFC 4035)  */
        static constexpr uint16_t RCODE_MASK{0x000F};   /**< Bits 3-0: RCODE (Response Code)          */
    }; // DnsHeaderFlagMasks
} // FilteringDnsResolver::Constants

#endif // DNS_HEADER_FLAG_MASKS_HPP

/*** end of file DnsHeaderFlagMasks.hpp ***/
