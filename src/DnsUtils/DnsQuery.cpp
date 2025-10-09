/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsQuery.cpp                                                  *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `DnsQuery` class, which       *
 *               represents a complete parsed DNS query message including      *
 *               header fields, domain name (QNAME), query type (QTYPE) and    *
 *               query class (QCLASS). It extends DnsHeader to provide a       *
 *               comprehensive representation of DNS query data structure      *
 *               for processing and validation purposes.                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsQuery.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DnsQuery` class for complete DNS query
 *        representation and data storage.
 */

#include "DnsUtils/DnsQuery.hpp"
#include "DnsUtils/DnsHeader.hpp"
#include "Utilities/Logger.hpp"
#include <utility>  // std::move
#include <string>   // std::string

using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsQuery::DnsQuery(const DnsHeader &header, const size_t qEndOffset,
                       string qName, const uint16_t qType, const uint16_t qClass)
        : mHeader{header},
          mQEndOffset{qEndOffset},
          mQName{move(qName)},
          mQType{qType},
          mQClass{qClass} {
        logger("DnsQuery::DnsQuery() constructor called with parameters:");
        logger("  Header: ID=%u, flags=0x%04X, qdcount=%u, ancount=SKIPPED, nscount=SKIPPED, arcount=SKIPPED",
               mHeader.mId, mHeader.mFlags, mHeader.mQdCount);
        logger("  Question end offset: %zu bytes", mQEndOffset);
        logger("  QNAME: '%s' (length=%zu)", mQName.c_str(), mQName.length());
        logger("  QTYPE: %u, QCLASS: %u", mQType, mQClass);

        verbose("DNS query object created for domain '%s' (type %u, class %u)",
                mQName.c_str(), mQType, mQClass);
        logger("DnsQuery object successfully constructed");
    } // DnsQuery::DnsQuery
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsQuery.cpp ***/
