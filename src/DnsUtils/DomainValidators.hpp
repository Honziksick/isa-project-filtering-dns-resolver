/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DomainValidators.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      12.10.2025                                                    *
 * Last edit:    12.10.2025                                                    *
 *                                                                             *
 * Description:  This header file declares the `DomainValidators` class,       *
 *               which provides methods for validating domain names according  *
 *               to DNS rules (RFC 1035). It ensures correct domain length,    *
 *               allowed characters, proper label separation, and label format.*
 *               Used for processing incoming DNS queries and filter contents. *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainValidators.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring the `DomainValidators` class for domain name
 *        validation in DNS queries and filter files.
 */

#ifndef DOMAIN_VALIDATORS_HPP
#define DOMAIN_VALIDATORS_HPP

#include <string_view>  // std::string_view

namespace FilteringDnsResolver::DnsUtils {
    class DomainValidators final {
    public:
        /**
         * @brief Validates total domain name length against DNS limits.
         *
         * @details Checks if domain name length complies with DNS maximum
         *          length restrictions to ensure proper network handling.
         *
         * @param domain String view of domain name to check.
         * @param isIncomingQName Boolean indicating if the domain is an incoming QName.
         */
        static void validateDomainLength(std::string_view domain, bool isIncomingQName);

        /**
         * @brief Validates domain character set compliance.
         *
         * @details Verifies that domain contains only valid DNS characters
         *          according to RFC specifications for domain names.
         *
         * @param domain String view of domain name to validate.
         * @param isIncomingQName Boolean indicating if the domain is an incoming QName.
         */
        static void validateDomainCharacters(std::string_view domain, bool isIncomingQName);

        /**
         * @brief Splits domain by dots and validates individual labels.
         *
         * @details Parses domain into labels separated by dots and validates
         *          each label for length and format compliance with DNS standards.
         *
         * @param domain String view of domain name to split and validate.
         * @param isIncomingQName Boolean indicating if the domain is an incoming QName.
         */
        static void splitByDotAndValidateLabels(std::string_view domain, bool isIncomingQName);

    private:
        /**
         * @brief Validates individual label length against DNS limits.
         *
         * @details Checks if domain label length meets DNS label size
         *          restrictions to ensure proper network protocol handling.
         *
         * @param label String view of domain label to check.
         * @param domain String view of full domain for error context.
         * @param isIncomingQName Boolean indicating if the domain is an incoming QName.
         */
        static void validateLabelLength(std::string_view label, std::string_view domain, bool isIncomingQName);

        /**
         * @brief Validates label format and character composition.
         *
         * @details Verifies that domain label follows proper DNS format
         *          including valid characters and hyphen placement rules.
         *
         * @param label String view of domain label to validate.
         * @param domain String view of full domain for error context.
         * @param isIncomingQName Boolean indicating if the domain is an incoming QName.
         */
        static void validateLabelFormat(std::string_view label, std::string_view domain, bool isIncomingQName);

        static constexpr auto DOT{'.'};  /**< Domain separator character for label parsing */
    }; // DnsValidators
} // FilteringDnsResolver::DnsUtils

#endif // DOMAIN_VALIDATORS_HPP

/*** end of file DomainValidators.hpp ***/
