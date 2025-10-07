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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsRCodes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

// https://help.dnsfilter.com/hc/en-us/articles/4408415850003-DNS-return-codes

#ifndef DNS_RCODES_HPP
#define DNS_RCODES_HPP

#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Enums
{
    enum class DnsRCodes : uint16_t {
        NOERROR  = 0,  /**< DNS Query completed successfully. */
        FORMERR  = 1,  /**< DNS Query Format Error. */
        SERVFAIL = 2,  /**< Server failed to complete the DNS request. */
        NXDOMAIN = 3,  /**< Domain name does not exist. */
        NOTIMP   = 4,  /**< Function not implemented. */
        REFUSED  = 5,  /**< The server refused to answer for the query. */
        YXDOMAIN = 6,  /**< Name that should not exist, does exist. */
        XRRSET   = 7,  /**< RRset that should not exist, does exist. */
        NOTAUTH  = 8,  /**< Server not authoritative for the zone. */
        NOTZONE  = 9,  /**< Name not in zone. */
    }; // DnsRCodes
} // FilteringDnsResolver::Enums

#endif // DNS_RCODES_HPP

/*** end of file DnsRCodes.hpp ***/
