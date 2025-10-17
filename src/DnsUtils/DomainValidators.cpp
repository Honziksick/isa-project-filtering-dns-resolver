/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DomainValidators.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      12.10.2025                                                    *
 * Last edit:    12.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `DomainValidators` class,     *
 *               which provides methods for validating domain names according  *
 *               to DNS rules (RFC 1035). It ensures correct domain length,    *
 *               allowed characters, proper label separation, and label format.*
 *               Used for processing incoming DNS queries and filter contents. *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainValidators.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DomainValidators` class for domain name
 *        validation in DNS queries and filter files.
 */

#include "DnsUtils/DomainValidators.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Constants/CustomLimits.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <string_view>  // std::string_view
#include <string>       // std::string
#include <cctype>       // std::isalnum()

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::DnsUtils {
    void DomainValidators::validateDomainLength(const string_view domain, const bool isIncomingQName) {
        const auto domainLength = domain.size();
        logger("DomainValidators::validateDomainLength() called: domain "
               "length=%zu (limit: 1-253)", domainLength);

        if(domainLength == 0 || domainLength >= CustomLimits::MAX_DOMAIN_LENGTH) {
            logger("ERROR: Domain length (%zu) is 0 or it exceeds maximum of 253 characters", domainLength);

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException(
                        "Domain length (" + to_string(domainLength) + ") out of "
                        "range (1-253 chars): '" + string(domain) + "'"
                        );
            }
        } // if

        logger("Domain length %zu is within valid range", domainLength);
    } // DomainValidators::validateDomainLength

    void DomainValidators::validateDomainCharacters(const string_view domain, const bool isIncomingQName) {
        logger("DomainValidators::validateDomainCharacters() called for domain: '%.*s'",
               static_cast<int>(domain.length()), domain.data());

        size_t characterIndex{1};
        for(const char character : domain) {
            if(!isalnum(static_cast<unsigned char>(character)) &&
                character != '-' && character != '.') {
                logger("ERROR: Invalid character '%c' found at position %zu", character, characterIndex);

                if(isIncomingQName) {
                    throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
                }
                else {
                    throw InvalidFilterFileContentException(
                            "Domain '" + string(domain) + "' contains an invalid "
                            "character '" + string(1, character) + "' at position '" +
                            to_string(characterIndex) + "'"
                            );
                }
            } // if

            characterIndex++;
        } // for

        logger("All %zu characters are valid (alphanumeric, hyphen, or dot)", characterIndex);
    } // DomainValidators::validateDomainCharacters

    void DomainValidators::splitByDotAndValidateLabels(const string_view domain, const bool isIncomingQName) {
        logger("DomainValidators::splitByDotAndValidateLabels() called for domain: "
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

                if(isIncomingQName) {
                    throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
                }
                else {
                    throw InvalidFilterFileContentException(
                            "Multiple consecutive dots detected in domain: '" + string(domain) + "'"
                            );
                }
            }

            // Extract the label and validate it
            const string_view label = domain.substr(start, end - start);
            logger("Extracted label %zu: '%.*s' (length=%zu)",
                   labelCount, static_cast<int>(label.length()), label.data(), label.length());

            validateLabelLength(label, domain, isIncomingQName);
            validateLabelFormat(label, domain, isIncomingQName);
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

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException(
                        "Multiple trailing dots detected in domain: '" + string(domain) + ".'"
                        );
            }
        }

        // Validate the last label
        validateLabelLength(remainder, domain, isIncomingQName);
        validateLabelFormat(remainder, domain, isIncomingQName);  // Format check must be called last as it uses .front() and .back()

        logger("Final label validation passed");
        logger("Domain contains %zu labels total", labelCount);
    } // DomainValidators::splitByDotAndValidateLabels

    void DomainValidators::validateLabelLength(const string_view label, const string_view domain, const bool isIncomingQName) {
        logger("DomainValidators::validateLabelLength() called for label: '%.*s' (length=%zu, limit: 1-63)",
               static_cast<int>(label.length()), label.data(), label.length());

        if(label.empty()) {
            logger("ERROR: Label is empty");

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException("Domain '" + string(domain) + "' contains an empty label");
            }
        }
        if(label.size() > CustomLimits::MAX_LABEL_LENGTH) {
            logger("ERROR: Label length %zu exceeds maximum of 63 characters", label.size());

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException(
                        "Domain '" + string(domain) + "' contains too long label (" +
                        to_string(label.size()) + "): '" + string(label) + "'"
                        );
            }
        }

        logger("Label length %zu is within valid range", label.size());
    } // DomainValidators::validateLabelLength

    void DomainValidators::validateLabelFormat(const string_view label, const string_view domain, const bool isIncomingQName) {
        logger("DomainValidators::validateLabelFormat() called for label: '%.*s'",
               static_cast<int>(label.length()), label.data());

        logger("Checking label boundaries: first='%c', last='%c'", label.front(), label.back());
        if(label.front() == '-' || label.back() == '-') {
            logger("ERROR: Label starts or ends with hyphen");

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException(
                        "Domain '" + string(domain) + "' contains a hyphen at "
                        "start or end in label: '" + string(label) + "'"
                        );
            }
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

            if(isIncomingQName) {
                throw DnsParseErrorException(DnsRCodes::FORMERR, "QNAME");
            }
            else {
                throw InvalidFilterFileContentException(
                        "Domain '" + string(domain) + "' contains an invalid character '" +
                        string(1, character) + "' in label '" + string(label) + "' at "
                        "position '" + to_string(charIndex) + "'"
                        );
            }
        } // for

        logger("All %zu characters in label are valid", charIndex);
    } // DomainValidators::validateLabelFormat
} // FilteringDnsResolver::DnsUtils

/*** end of file DomainValidators.cpp ***/
