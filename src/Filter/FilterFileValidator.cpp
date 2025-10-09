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
 * Last edit:    01.10.2025                                                    *
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
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <string_view>  // std::string_view
#include <string>       // std::string
#include <cctype>       // std::isalnum()

using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::Filter
{
    void FilterFileValidator::validateLine(const string_view line) {
        // Validate overall domain length and allowed characters
        validateDomainLength(line);
        validateDomainCharacters(line);

        // Split the domain into labels and validate each label
        splitByDotAndValidateLabels(line);
    } // FilterFileValidator::validateLine

    void FilterFileValidator::validateDomainLength(const string_view domain) {
        const auto domainLength = domain.size();
        if(domainLength == 0 || domainLength >= 254) {
            throw InvalidFilterFileContentException(
                    "Domain length (" + to_string(domainLength) + ") out of "
                    "range (1-253 chars): '" + string(domain) + "'"
                    );
        } // if
    } // FilterFileValidator::validateDomainLength

    void FilterFileValidator::validateDomainCharacters(const string_view domain) {
        for(const char character : domain) {
            if(!isalnum(static_cast<unsigned char>(character)) &&
                character != '-' && character != '.') {
                throw InvalidFilterFileContentException(
                        "Domain '" + string(domain) + "' contains an invalid "
                        "character '" + string(1, character) + "'"
                        );
            } // if
        } // for
    } // FilterFileValidator::validateDomainCharacters

    void FilterFileValidator::splitByDotAndValidateLabels(const string_view domain) {
        size_t start = 0;               // Start position for substring extraction
        size_t end = domain.find(DOT);  // End is the position of the first occurrence of dot

        // Loop until the end of the line
        while(end != string_view::npos) {
            // If two dots are next to each other, the label is empty
            if(end == start) {
                throw InvalidFilterFileContentException(
                        "Multiple consecutive dots detected in domain: '" + string(domain) + "'"
                        );
            }

            // Extract the label and validate it
            const string_view label = domain.substr(start, end - start);
            validateLabelLength(label, domain);
            validateLabelFormat(label, domain);

            start = end + 1; // +1 to skip the dot
            end = domain.find(DOT, start);
        }

        // If the last label is empty, there were two trailing dots
        const std::string_view remainder = domain.substr(start);
        if(remainder.empty()) {
            throw InvalidFilterFileContentException(
                    "Multiple trailing dots detected in domain: '" + string(domain) + "'"
                    );
        }

        // Validate the last label
        validateLabelLength(remainder, domain);
        validateLabelFormat(remainder, domain);  // Format check must be called last as it uses .front() and .back()
    } // FilterFileValidator::splitByDotAndValidateLabels

    void FilterFileValidator::validateLabelLength(const string_view label, const string_view domain) {
        if(label.empty()) {
            throw InvalidFilterFileContentException("Domain '" + string(domain) + "' contains an empty label");
        }
        if(label.size() > 63) {
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains too long label (" +
                    to_string(label.size()) + "): '" + string(label) + "'"
                    );
        }
    } // FilterFileValidator::validateLabelLength

    void FilterFileValidator::validateLabelFormat(const string_view label, const string_view domain) {
        if(label.front() == '-' || label.back() == '-') {
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains a hyphen at "
                    "start or end in label: '" + string(label) + "'"
                    );
        }
        for(const char character : label) {
            if(isalnum(static_cast<unsigned char>(character)) || character == '-') {
                continue;
            }
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains an invalid character '" +
                    string(1, character) + "' in label '" + string(label) + "'"
                    );
        }
    } // FilterFileValidator::validateLabelFormat
} // FilteringDnsResolver::Filter

/*** end of file FilterFileValidator.cpp ***/
