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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainFilter.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "Filter/DomainFilter.hpp"
#include <string_view>  // std::string_view
#include <utility>      // std::move

using namespace std;

namespace FilteringDNSResolver::Filter
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
} // FilteringDNSResolver::Filter

/*** end of file DomainFilter.cpp ***/
