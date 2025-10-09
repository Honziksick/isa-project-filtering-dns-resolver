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
#include "Utilities/Logger.hpp"
#include <string_view>  // std::string_view
#include <utility>      // std::move

using namespace std;

namespace FilteringDnsResolver::Filter
{
    DomainFilter::DomainFilter(vector<string> filterFileContent) {
        logger("DomainFilter::DomainFilter() constructor called with "
               "%zu domain entries", filterFileContent.size());
        verbose("Loading domain filter with %zu blocked domains",
                filterFileContent.size());

        size_t processedCount = 0;
        size_t emptyCount = 0;

        for(auto &domainName : filterFileContent) {
            if(domainName.empty()) {
                emptyCount++;
                logger("Skipping empty domain entry at index %zu", processedCount);
                continue;
            }

            logger("Processing domain entry %zu: '%s' (length=%zu)",
                   processedCount, domainName.c_str(), domainName.length());
            mDomainNameSet.insert(move(domainName));
            processedCount++;
        } // for

        logger("DomainFilter construction completed: %zu domains loaded, "
               "%zu empty entries skipped", processedCount, emptyCount);
        logger("Final hash set size: %zu entries, load_factor=%.2f",
               mDomainNameSet.size(), mDomainNameSet.load_factor());
        verbose("Domain filter ready - blocking %zu domains", mDomainNameSet.size());
        logger("DomainFilter object constructed");
    } // DomainFilter::DomainFilter

    bool DomainFilter::domainMatches(string_view domainName) const {
        logger("DomainFilter::domainMatches() called for domain: '%.*s' (length=%zu)",
               static_cast<int>(domainName.length()), domainName.data(), domainName.length());

        // Check for exact match
        logger("Checking exact match for domain '%.*s'",
               static_cast<int>(domainName.length()), domainName.data());
        if(mDomainNameSet.contains(domainName)) {
            logger("Exact match found for domain '%.*s'",
                   static_cast<int>(domainName.length()), domainName.data());
            verbose("Domain '%.*s' blocked (exact match)",
                    static_cast<int>(domainName.length()), domainName.data());
            return true;
        }
        logger("No exact match found for domain '%.*s'",
               static_cast<int>(domainName.length()), domainName.data());

        // Check for suffix match
        logger("Starting suffix matching for domain '%.*s'",
               static_cast<int>(domainName.length()), domainName.data());
        size_t dotPosition = domainName.find(DOT);
        size_t suffixAttempts = 0;

        while(dotPosition != string_view::npos) {
            const string_view suffix = domainName.substr(dotPosition + 1);
            suffixAttempts++;
            logger("Suffix attempt %zu: checking '%.*s' at dot position %zu",
                   suffixAttempts, static_cast<int>(suffix.length()), suffix.data(), dotPosition);

            if(mDomainNameSet.contains(suffix)) {
                logger("Suffix match found: '%.*s' matches blocked domain",
                       static_cast<int>(suffix.length()), suffix.data());
                verbose("Domain '%.*s' blocked (subdomain of '%.*s')",
                        static_cast<int>(domainName.length()), domainName.data(),
                        static_cast<int>(suffix.length()), suffix.data());
                return true;
            }
            logger("Suffix '%.*s' not found in blocked domains",
                   static_cast<int>(suffix.length()), suffix.data());

            dotPosition = domainName.find(DOT, dotPosition + 1);  // We start searching after the current dot (+1)
        } // while

        logger("Suffix matching completed: checked %zu suffixes, no matches found", suffixAttempts);
        logger("Domain '%.*s' is NOT blocked (no matches found)",
               static_cast<int>(domainName.length()), domainName.data());
        verbose("Domain '%.*s' allowed through filter",
                static_cast<int>(domainName.length()), domainName.data());

        // No match found
        return false;
    } // DomainFilter::domainMatches
} // FilteringDnsResolver::Filter

/*** end of file DomainFilter.cpp ***/
