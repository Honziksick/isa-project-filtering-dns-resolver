/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsOpcodes.hpp                                                *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    09.10.2025                                                    *
 *                                                                             *
 * Description:  This header file defines the `DnsOpcodes` enum class for      *
 *               strongly typed DNS operation codes according to RFC 1035.     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsOpcodes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring strongly typed DNS opcode enumeration.
 *
 * @note This implementation was inspired by (section DNS OpCodes):
 *       https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-5
 */

#ifndef DNS_OPCODES_HPP
#define DNS_OPCODES_HPP

#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Enums
{
    /**
     * @enum DnsOpcodes
     * @brief Strongly typed enumeration for DNS operation codes.
     *
     * @details Defines DNS operation codes as specified in RFC 1035
     *          using strongly typed enum class for type safety and
     *          better compile-time checking.
     */
    enum class DnsOpcodes : uint16_t {
        QUERY = 0,           /**< Standard DNS query operation.                    */
        IQUERY = 1,          /**< Inverse query operation (obsolete per RFC 3425). */
        STATUS = 2,          /**< Server status request operation.                 */
        UNASSIGNED_3 = 3,    /**< Unassigned opcode value 3.                       */
        NOTIFY = 4,          /**< DNS NOTIFY operation (RFC 1996).                 */
        UPDATE = 5,          /**< DNS dynamic update operation (RFC 2136).         */
        STATEFUL_OPS = 6,    /**< Reserved opcode value 6.                         */
        UNASSIGNED_7 = 7,    /**< Unassigned opcode value 7.  */
        UNASSIGNED_8 = 8,    /**< Unassigned opcode value 8.  */
        UNASSIGNED_9 = 9,    /**< Unassigned opcode value 9.  */
        UNASSIGNED_10 = 10,  /**< Unassigned opcode value 10. */
        UNASSIGNED_11 = 11,  /**< Unassigned opcode value 11. */
        UNASSIGNED_12 = 12,  /**< Unassigned opcode value 12. */
        UNASSIGNED_13 = 13,  /**< Unassigned opcode value 13. */
        UNASSIGNED_14 = 14,  /**< Unassigned opcode value 14. */
        UNASSIGNED_15 = 15,  /**< Unassigned opcode value 15. */
    }; // DnsOpcodes
} // FilteringDnsResolver::Enums

#endif // DNS_OPCODES_HPP

/*** end of file DnsOpcodes.hpp ***/
