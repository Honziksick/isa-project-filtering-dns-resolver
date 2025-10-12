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
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <string_view>  // std::string_view
#include <string>       // std::string
#include <cctype>       // std::isalnum()

using namespace FilteringDnsResolver::Exceptions;
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
        validateDomainLength(domainToValidate);
        logger("Domain length validation passed");

        logger("Validating domain characters");
        validateDomainCharacters(domainToValidate);
        logger("Domain characters validation passed");

        // Split the domain into labels and validate each label
        logger("Splitting domain into labels and validating each");
        splitByDotAndValidateLabels(domainToValidate);
        logger("Label validation completed successfully");

        logger("%s validation completed for: '%.*s' (length=%zu)",
               isWildcardPattern ? "Wildcard" : "Domain",
               static_cast<int>(line.length()), line.data(), line.length());

        return !isWildcardPattern;  // true => exact domain, false => wildcard
    } // FilterFileValidator::validateLine

    void FilterFileValidator::validateDomainLength(const string_view domain) {
        const auto domainLength = domain.size();
        logger("FilterFileValidator::validateDomainLength() called: domain "
               "length=%zu (limit: 1-253)", domainLength);

        if(domainLength == 0 || domainLength >= 254) {
            logger("ERROR: Domain length (%zu) is 0 or it exceeds maximum of 253 characters", domainLength);
            throw InvalidFilterFileContentException(
                    "Domain length (" + to_string(domainLength) + ") out of "
                    "range (1-253 chars): '" + string(domain) + "'"
                    );
        } // if

        logger("Domain length %zu is within valid range", domainLength);
    } // FilterFileValidator::validateDomainLength

    bool FilterFileValidator::isWildcard(const string_view domain) {
        return (domain.length() >= 2) && (domain[0] == '*') && (domain[1] == '.');
    } // FilterFileValidator::isWildcard

    void FilterFileValidator::validateDomainCharacters(const string_view domain) {
        logger("FilterFileValidator::validateDomainCharacters() called for domain: '%.*s'",
               static_cast<int>(domain.length()), domain.data());

        size_t characterIndex = 0;
        for(const char character : domain) {
            if(!isalnum(static_cast<unsigned char>(character)) &&
                character != '-' && character != '.') {
                logger("ERROR: Invalid character '%c' found at position %zu", character, characterIndex);
                throw InvalidFilterFileContentException(
                        "Domain '" + string(domain) + "' contains an invalid "
                        "character '" + string(1, character) + "' at position '" +
                        to_string(characterIndex) + "'"
                        );
            } // if

            characterIndex++;
        } // for

        logger("All %zu characters are valid (alphanumeric, hyphen, or dot)", characterIndex);
    } // FilterFileValidator::validateDomainCharacters

    void FilterFileValidator::splitByDotAndValidateLabels(const string_view domain) {
        logger("FilterFileValidator::splitByDotAndValidateLabels() called for domain: "
               "'%.*s'", static_cast<int>(domain.length()), domain.data());

        size_t start = 0;               // Start position for substring extraction
        size_t end = domain.find(DOT);  // End is the position of the first occurrence of dot
        size_t labelCount = 0;

        logger("Starting label extraction - first dot at position: %s",
               (end != string_view::npos) ? to_string(end).c_str() : "not found");

        // Loop until the end of the line
        while(end != string_view::npos) {
            labelCount++;
            logger("Processing label %zu: positions %zu-%zu", labelCount, start, end);

            // If two dots are next to each other, the label is empty
            if(end == start) {
                logger("ERROR: Consecutive dots detected at position %zu", start);
                throw InvalidFilterFileContentException(
                        "Multiple consecutive dots detected in domain: '" + string(domain) + "'"
                        );
            }

            // Extract the label and validate it
            const string_view label = domain.substr(start, end - start);
            logger("Extracted label %zu: '%.*s' (length=%zu)",
                   labelCount, static_cast<int>(label.length()), label.data(), label.length());

            validateLabelLength(label, domain);
            validateLabelFormat(label, domain);
            logger("Label %zu validation passed", labelCount);

            start = end + 1; // +1 to skip the dot
            end = domain.find(DOT, start);

            logger("Next search starting at position %zu, next dot at: %s",
                   start, (end != string_view::npos) ? to_string(end).c_str() : "not found");
        } // while

        // If the last label is empty, there were two trailing dots
        const std::string_view remainder = domain.substr(start);
        labelCount++;
        logger("Processing final label %zu: '%.*s' (length=%zu)",
               labelCount, static_cast<int>(remainder.length()), remainder.data(), remainder.length());

        if(remainder.empty()) {
            logger("ERROR: Empty final label detected (trailing dots)");
            throw InvalidFilterFileContentException(
                    "Multiple trailing dots detected in domain: '" + string(domain) + "'"
                    );
        }

        // Validate the last label
        validateLabelLength(remainder, domain);
        validateLabelFormat(remainder, domain);  // Format check must be called last as it uses .front() and .back()

        logger("Final label validation passed");
        logger("Domain contains %zu labels total", labelCount);
    } // FilterFileValidator::splitByDotAndValidateLabels

    void FilterFileValidator::validateLabelLength(const string_view label, const string_view domain) {
        logger("FilterFileValidator::validateLabelLength() called for label: '%.*s' (length=%zu, limit: 1-63)",
               static_cast<int>(label.length()), label.data(), label.length());

        if(label.empty()) {
            logger("ERROR: Label is empty");
            throw InvalidFilterFileContentException("Domain '" + string(domain) + "' contains an empty label");
        }
        if(label.size() > 63) {
            logger("ERROR: Label length %zu exceeds maximum of 63 characters", label.size());
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains too long label (" +
                    to_string(label.size()) + "): '" + string(label) + "'"
                    );
        }

        logger("Label length %zu is within valid range", label.size());
    } // FilterFileValidator::validateLabelLength

    void FilterFileValidator::validateLabelFormat(const string_view label, const string_view domain) {
        logger("FilterFileValidator::validateLabelFormat() called for label: '%.*s'",
               static_cast<int>(label.length()), label.data());

        logger("Checking label boundaries: first='%c', last='%c'", label.front(), label.back());
        if(label.front() == '-' || label.back() == '-') {
            logger("ERROR: Label starts or ends with hyphen");
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains a hyphen at "
                    "start or end in label: '" + string(label) + "'"
                    );
        }
        logger("Label boundaries are valid (no leading/trailing hyphens)");

        logger("Checking all characters in label");
        size_t charIndex = 0;
        for(const char character : label) {
            if(isalnum(static_cast<unsigned char>(character)) || character == '-') {
                charIndex++;
                continue;
            }

            logger("ERROR: Invalid character '%c' in label at position %zu", character, charIndex);
            throw InvalidFilterFileContentException(
                    "Domain '" + string(domain) + "' contains an invalid character '" +
                    string(1, character) + "' in label '" + string(label) + "' at "
                    "position '" + to_string(charIndex) + "'"
                    );
        } // for

        logger("All %zu characters in label are valid", charIndex);
    } // FilterFileValidator::validateLabelFormat
} // FilteringDnsResolver::Filter

/*** end of file FilterFileValidator.cpp ***/
