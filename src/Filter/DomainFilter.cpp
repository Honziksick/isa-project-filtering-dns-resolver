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
    DomainFilter::DomainFilter(vector<string> exactDomains, vector<string> wildcardPatterns) {
        logger("DomainFilter::DomainFilter() constructor called with %zu exact domains and %zu wildcards",
               exactDomains.size(), wildcardPatterns.size());
        verbose("Loading domain filter with %zu exact domains and %zu wildcard patterns",
                exactDomains.size(), wildcardPatterns.size());

        size_t processedExact = 0;
        size_t processedWildcards = 0;
        size_t emptyExact = 0;
        size_t emptyWildcards = 0;

        // Process exact domains
        logger("Processing exact domains (%zu entries)", exactDomains.size());
        for(auto &domainName : exactDomains) {
            if(domainName.empty()) {
                emptyExact++;
                logger("Skipping empty exact domain entry at index %zu", processedExact);
                continue;
            }

            logger("Processing exact domain %zu: '%s' (length=%zu)",
                   processedExact, domainName.c_str(), domainName.length());
            mDomainNameSet.insert(move(domainName));
            processedExact++;
        } // for

        // Process wildcard patterns
        logger("Processing wildcard patterns (%zu entries)", wildcardPatterns.size());
        for(auto &wildcardPattern : wildcardPatterns) {
            if(wildcardPattern.empty()) {
                emptyWildcards++;
                logger("Skipping empty wildcard entry at index %zu", processedWildcards);
                continue;
            }

            logger("Processing wildcard pattern %zu: '%s' (length=%zu)",
                   processedWildcards, wildcardPattern.c_str(), wildcardPattern.length());
            mWildcardSet.insert(move(wildcardPattern));
            processedWildcards++;
        } // for

        logger("DomainFilter construction completed: %zu exact domains, %zu wildcards loaded, "
               "%zu empty exact entries, %zu empty wildcard entries skipped",
               processedExact, processedWildcards, emptyExact, emptyWildcards);
        logger("Final hash sets - exact domains: %zu entries (load_factor=%.2f), "
               "wildcards: %zu entries (load_factor=%.2f)",
               mDomainNameSet.size(), mDomainNameSet.load_factor(),
               mWildcardSet.size(), mWildcardSet.load_factor());
        verbose("Domain filter ready - blocking %zu exact domains and %zu wildcard patterns",
                mDomainNameSet.size(), mWildcardSet.size());
        logger("DomainFilter object constructed");
    } // DomainFilter::DomainFilter

    bool DomainFilter::domainMatches(string_view domainName) const {
        logger("DomainFilter::domainMatches() called for domain: '%.*s' (length=%zu)",
               static_cast<int>(domainName.length()), domainName.data(), domainName.length());

        // Remove trailing dot if present
        removeTrailingDot(domainName);

        // Check for wildcard match first
        if(exactDomainMatches(domainName) || wildcardMatches(domainName)) {
            return true;
        }

        logger("Domain '%.*s' is NOT blocked (no exact matches or wildcards found)",
               static_cast<int>(domainName.length()), domainName.data());
        verbose("Domain '%.*s' allowed through filter",
                static_cast<int>(domainName.length()), domainName.data());

        // Check for full domain and subdomain matches
        return false;
    } // DomainFilter::domainMatches

    bool DomainFilter::wildcardMatches(string_view domainName) const {
        logger("DomainFilter::wildcardMatches() called for WILDCARD: '%.*s' (length=%zu)",
               static_cast<int>(domainName.length()), domainName.data(), domainName.length());

        size_t dotPosition = domainName.find(DOT);  // find the first dot
        size_t wildcardAttempts = 0;  // counter for logging purposes

        // Iterate through each dot position to construct wildcard patterns
        while(dotPosition != string_view::npos) {
            const string_view suffix = domainName.substr(dotPosition + 1);
            wildcardAttempts++;

            // We construct a wildcard form "*.suffix"
            string wildcardPattern = "*.";
            wildcardPattern.append(suffix);

            logger("WILDCARD attempt %zu: checking pattern '%s'",
                   wildcardAttempts, wildcardPattern.c_str());

            // Check if the constructed wildcard pattern exists in the wildcard set
            if(mWildcardSet.contains(wildcardPattern)) {
                logger("WILDCARD match found: pattern '%s' matches domain",
                       wildcardPattern.c_str());
                verbose("Domain '%.*s' blocked (wildcard match '%s')",
                        static_cast<int>(domainName.length()), domainName.data(),
                        wildcardPattern.c_str());
                return true;
            }

            dotPosition = domainName.find(DOT, dotPosition + 1);  // move to the next dot
        }

        logger("No WILDCARD matches found after %zu attempts", wildcardAttempts);
        return false;
    } // DomainFilter::wildcardMatches

    bool DomainFilter::exactDomainMatches(string_view domainName) const {
        logger("DomainFilter::exactDomainMatches() called for EXACT DOMAIN: '%.*s' (length=%zu)",
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
        logger("No exact match found for EXACT domain '%.*s'",
               static_cast<int>(domainName.length()), domainName.data());

        // Check for suffix match
        logger("Starting suffix matching for EXACT domain '%.*s'",
               static_cast<int>(domainName.length()), domainName.data());
        size_t dotPosition = domainName.find(DOT);
        size_t suffixAttempts = 0;

        while(dotPosition != string_view::npos) {
            const string_view suffix = domainName.substr(dotPosition + 1);
            suffixAttempts++;
            logger("Suffix attempt %zu: checking '%.*s' at dot position %zu",
                   suffixAttempts, static_cast<int>(suffix.length()), suffix.data(), dotPosition);

            if(mDomainNameSet.contains(suffix)) {
                logger("Suffix match found: '%.*s' matches blocked EXACT domain",
                       static_cast<int>(suffix.length()), suffix.data());
                verbose("Domain '%.*s' blocked (subdomain of '%.*s')",
                        static_cast<int>(domainName.length()), domainName.data(),
                        static_cast<int>(suffix.length()), suffix.data());
                return true;
            }
            logger("Suffix '%.*s' not found in blocked EXACT domains",
                   static_cast<int>(suffix.length()), suffix.data());

            dotPosition = domainName.find(DOT, dotPosition + 1);  // We start searching after the current dot (+1)
        } // while

        logger("Suffix matching completed: checked %zu suffixes, no matches found", suffixAttempts);

        // No match found
        return false;
    } // DomainFilter::exactDomainMatches

    void DomainFilter::removeTrailingDot(string_view &domain) {
        if(!domain.empty() && domain.back() == '.') {
            domain = domain.substr(0, domain.length() - 1);
        }
    } // DomainFilter::removeTrailingDot
} // FilteringDnsResolver::Filter

/*** end of file DomainFilter.cpp ***/
