/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsRCodes.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsRCodes` enumeration which       *
 *               defines standard DNS response codes (RCODE) according to      *
 *               RFC 1035 and subsequent RFCs. These codes indicate the        *
 *               status of DNS query processing and are used in DNS response   *
 *               messages to communicate success, various error conditions,    *
 *               and server state information to DNS clients.                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsRCodes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsRCodes` enumeration for standard DNS
 *        response codes and status indication.
 *
 * @note This implementation was inspired by:
 * https://help.dnsfilter.com/hc/en-us/articles/4408415850003-DNS-return-codes
 */

#ifndef DNS_RCODES_HPP
#define DNS_RCODES_HPP

#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Enums
{
    /**
     * @enum DnsRCodes
     * @brief Standard DNS response codes indicating query processing status.
     *
     * @details Defines RCODE values used in DNS response messages according
     *          to RFC 1035 and subsequent specifications. These codes communicate
     *          the result of DNS query processing to clients, including success
     *          states and various error conditions.
     */
    enum class DnsRCodes : uint16_t {
        NOERROR  = 0,  /**< DNS Query completed successfully.           */
        FORMERR  = 1,  /**< DNS Query Format Error.                     */
        SERVFAIL = 2,  /**< Server failed to complete the DNS request.  */
        NXDOMAIN = 3,  /**< Domain name does not exist.                 */
        NOTIMP   = 4,  /**< Function not implemented.                   */
        REFUSED  = 5,  /**< The server refused to answer for the query. */
        YXDOMAIN = 6,  /**< Name that should not exist, does exist.     */
        XRRSET   = 7,  /**< RRset that should not exist, does exist.    */
        NOTAUTH  = 8,  /**< Server not authoritative for the zone.      */
        NOTZONE  = 9,  /**< Name not in zone.                           */
    }; // DnsRCodes
} // FilteringDnsResolver::Enums

#endif // DNS_RCODES_HPP

/*** end of file DnsRCodes.hpp ***/
