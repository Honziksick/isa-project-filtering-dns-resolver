/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         CustomLimits.hpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    07.10.2025                                                    *
 *                                                                             *
 * Description: Header file declaring project specific numeric limits used     *
 *              across modules, especially for DNS related value ranges.       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file CustomLimits.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file containing numeric ranges used within the Filtering
 *        DNS Resolver (e.g., valid DNS Transaction ID range).
 */

#ifndef CUSTOM_LIMITS_HPP
#define CUSTOM_LIMITS_HPP

#include <cstdint>  // uint16_t
#include <limits>   // std::numeric_limits

namespace FilteringDnsResolver::Constants
{
    /**
     * @class CustomLimits
     * @brief Class containing numeric ranges used within the Filtering
     *        DNS Resolver (e.g., valid DNS Transaction ID range).
     */
    class CustomLimits {
    public:
        // Server port range
        static constexpr size_t MIN_SERVER_PORT = 1;  /**< Minimum server port number (port 0 is reserved and thus not allowed). */
        static constexpr size_t MAX_SERVER_PORT = std::numeric_limits<uint16_t>::max();  /**< Maximum server port number.        */

        // Domains and labels
        static constexpr size_t MAX_DOMAIN_LENGTH = 253;  /**< Maximum length of a full domain name (in characters).             */
        static constexpr size_t MAX_LABEL_LENGTH = 63;    /**< Maximum length of a single label within a domain (in characters). */

        // DNS Transaction ID (TXID) range
        /**
         * @brief Minimum inclusive 16-bit DNS Transaction ID value.
         * @details Valid TXID values span the full 16-bit unsigned range.
         */
        static constexpr uint16_t MIN_TX_ID16 = std::numeric_limits<uint16_t>::min();

        /**
         * @brief Maximum inclusive 16-bit DNS Transaction ID value.
         * @details Used with `uniform_int_distribution` to ensure the entire
         *          16-bit TXID space (0-65535) is reachable.
         */
        static constexpr uint16_t MAX_TX_ID16 = std::numeric_limits<uint16_t>::max();

        // OTHERS
        static constexpr size_t MAX_DNS_UDP_MESSAGE_SIZE{512};  /**< Maximum DNS message size over UDP as per RFC 1035. */
    }; // CustomLimits
} // FilteringDnsResolver::Constants

#endif // CUSTOM_LIMITS_HPP

/*** end of file CustomLimits.hpp ***/
