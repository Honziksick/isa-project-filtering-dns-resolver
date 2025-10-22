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
 * Last edit:    17.10.2025                                                    *
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
#include <vector>   // std::vector

using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsQuery::DnsQuery(const DnsHeader &header, const size_t qEndOffset,
                       vector<string> qNames, vector<uint16_t> qTypes, vector<uint16_t> qClasses)
        : mHeader{header},
          mQEndOffset{qEndOffset},
          mQNames{move(qNames)},
          mQTypes{move(qTypes)},
          mQClasses{move(qClasses)} {
        // Helper lambdas for logging
        auto concatStrings = [](const vector<string> &stringVector) {
            string result;
            for(size_t iString = 0; iString < stringVector.size(); iString++) {
                if(iString > 0) {
                    result += ", ";
                }
                result += stringVector[iString];
            }
            return result;
        };
        auto joinU16 = [](const vector<uint16_t> &U16Vector) {
            string result;
            for(size_t iU16 = 0; iU16 < U16Vector.size(); iU16++) {
                if(iU16 > 0) {
                    result += ", ";
                }
                result += to_string(U16Vector[iU16]);
            }
            return result;
        };

        logger("  QNAMES: [%s] (count=%zu)", concatStrings(mQNames).c_str(), mQNames.size());
        logger("  QTYPES: [%s]", joinU16(mQTypes).c_str());
        logger("  QCLASSES: [%s]", joinU16(mQClasses).c_str());
        verbose("DNS query object created for domains [%s] (types [%s], classes [%s])",
                concatStrings(mQNames).c_str(), joinU16(mQTypes).c_str(), joinU16(mQClasses).c_str());

        logger("DnsQuery object successfully constructed");
    } // DnsQuery::DnsQuery
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsQuery.cpp ***/
