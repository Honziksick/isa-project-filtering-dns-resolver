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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsQuery.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "DnsUtils/DnsQuery.hpp"
#include "DnsUtils/DnsHeader.hpp"
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
          mQClass{qClass} {}
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsQuery.cpp ***/
