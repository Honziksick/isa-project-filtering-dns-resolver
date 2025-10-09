/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DomainFilter.cpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      01.10.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  This soruce file implements the `DomainFilter` class, which   *
 *               implements efficient domain name filtering for the DNS        *
 *               resolver. It provides fast domain matching using hash-based   *
 *               lookups with heterogeneous access support for `string_view`   *
 *               queries. The filter supports subdomain matching and uses      *
 *               optimized data structures for high-performance DNS query      *
 *               filtering in real-time network applications.                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainFilter.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DomainFilter` class for efficient
 *        domain name filtering and DNS query matching functionality.
 *
 * @note This implementation was inspired by:
 *       https://www.cppstories.com/2021/heterogeneous-access-cpp20/
 *       and
 *       https://ebadblog.com/looking-up-a-c++-hash-table-with-a-pre-known-hash
 */

#include "Filter/DomainFilter.hpp"
#include <string_view>  // std::string_view
#include <utility>      // std::move

using namespace std;

namespace FilteringDnsResolver::Filter
{
    DomainFilter::DomainFilter(vector<string> filterFileContent) {
        for(auto &domainName : filterFileContent) {
            mDomainNameSet.insert(move(domainName));
        }
    } // DomainFilter::DomainFilter

    bool DomainFilter::domainMatches(string_view domainName) const {
        // Check for exact match
        if(mDomainNameSet.contains(domainName)) {
            return true;
        }

        // Check for suffix match
        size_t dotPosition = domainName.find(DOT);
        while(dotPosition != string_view::npos) {
            if(mDomainNameSet.contains(domainName.substr(dotPosition + 1))) {
                return true;
            }

            dotPosition = domainName.find(DOT, dotPosition + 1);  // We start searching after the current dot (+1)
        }

        // No match found
        return false;
    } // DomainFilter::domainMatches
} // FilteringDnsResolver::Filter

/*** end of file DomainFilter.cpp ***/
