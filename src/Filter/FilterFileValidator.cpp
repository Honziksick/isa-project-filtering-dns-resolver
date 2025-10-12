/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFileValidator.cpp                                       *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    11.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `FilterFileValidator` class,  *
 *               which does comprehensive domain name validation for filter    *
 *               configuration files. It validates domain format compliance    *
 *               with DNS standards including length limits, character sets,   *
 *               and label structure. The validator ensures all domains in     *
 *               filter files meet RFC specifications before being used in     *
 *               the DNS resolver filtering engine for reliable operation.     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileValidator.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `FilterFileValidator` class for domain
 *        name validation and DNS standard compliance checking functionality.
 */

#include "Filter/FilterFileValidator.hpp"
#include "DnsUtils/DomainValidators.hpp"
#include "Utilities/Logger.hpp"
#include <string_view>  // std::string_view

using namespace FilteringDnsResolver::DnsUtils;
using namespace std;

namespace FilteringDnsResolver::Filter
{
    bool FilterFileValidator::validateLine(const string_view line) {
        logger("FilterFileValidator::validateLine() called with domain: '%.*s' (length=%zu)",
               static_cast<int>(line.length()), line.data(), line.length());

        // Check if it's a wildcard pattern (starts with "*.")
        const bool isWildcardPattern = isWildcard(line);
        const string_view domainToValidate = isWildcardPattern ? line.substr(2) : line;

        if(isWildcardPattern) {
            logger("Detected wildcard pattern, validating suffix: '%.*s'",
                   static_cast<int>(domainToValidate.length()), domainToValidate.data());
        }
        else {
            logger("Validating exact domain (not wildcard): '%.*s'",
                   static_cast<int>(domainToValidate.length()), domainToValidate.data());
        }

        // Validate overall domain length and allowed characters
        logger("Validating domain length");
        DomainValidators::validateDomainLength(domainToValidate, false);
        logger("Domain length validation passed");

        logger("Validating domain characters");
        DomainValidators::validateDomainCharacters(domainToValidate, false);
        logger("Domain characters validation passed");

        // Split the domain into labels and validate each label
        logger("Splitting domain into labels and validating each");
        DomainValidators::splitByDotAndValidateLabels(domainToValidate, false);
        logger("Label validation completed successfully");

        logger("%s validation completed for: '%.*s' (length=%zu)",
               isWildcardPattern ? "Wildcard" : "Domain",
               static_cast<int>(line.length()), line.data(), line.length());

        return !isWildcardPattern;  // true => exact domain, false => wildcard
    } // FilterFileValidator::validateLine

    bool FilterFileValidator::isWildcard(const string_view domain) {
        return (domain.length() >= 2) && (domain[0] == '*') && (domain[1] == '.');
    } // FilterFileValidator::isWildcard
} // FilteringDnsResolver::Filter

/*** end of file FilterFileValidator.cpp ***/
