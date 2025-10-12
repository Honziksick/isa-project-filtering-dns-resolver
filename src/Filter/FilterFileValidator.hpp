/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFileValidator.hpp                                       *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    11.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `FilterFileValidator` class, which  *
 *               implements comprehensive domain name validation for filter    *
 *               configuration files. It validates domain format compliance    *
 *               with DNS standards including length limits, character sets,   *
 *               and label structure. The validator ensures all domains in     *
 *               filter files meet RFC specifications before being used in     *
 *               the DNS resolver filtering engine for reliable operation.     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileValidator.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `FilterFileValidator` class for domain name
 *        validation and DNS standard compliance checking functionality.
 */

#ifndef FILTER_FILE_VALIDATOR_HPP
#define FILTER_FILE_VALIDATOR_HPP

#include <string_view>  // std::string_view

namespace FilteringDnsResolver::Filter
{
    /**
     * @class FilterFileValidator
     * @brief Base class for domain name validation and DNS compliance checking.
     *
     * @details Provides comprehensive validation functionality for domain names
     *          in filter configuration files. Validates domain format against
     *          DNS standards including length constraints, character sets, and
     *          label structure to ensure reliable filtering operations.
     */
    class FilterFileValidator {
    protected:
        /**
         * @brief Validates a single line (domain or wildcard pattern) from filter file.
         *
         * @details Performs comprehensive validation of domain name format
         *          including length, character set, and label structure
         *          according to DNS standards (RFC 1035). Supports wildcard
         *          patterns starting with "*.".
         *
         * @param line Domain name or wildcard pattern to validate.
         *
         * @return `true` if it's an exact domain, `false` if it's a wildcard pattern.
         *
         * @throws InvalidFilterFileContentException If domain format is invalid.
         */
        [[nodiscard]]
        static bool validateLine(std::string_view line);

    private:
        /**
         * @brief Validates total domain name length against DNS limits.
         *
         * @details Checks if domain name length complies with DNS maximum
         *          length restrictions to ensure proper network handling.
         *
         * @param domain String view of domain name to check.
         */
        static void validateDomainLength(std::string_view domain);

        /**
         * @brief Checks if the domain is a wildcard pattern.
         *
         * @details Determines if the provided domain string represents
         *          a wildcard pattern by checking for the "*." prefix.
         *
         * @param domain String view of domain name to check.
         *
         * @return `true` if domain is a wildcard pattern, `false` otherwise.
         */
        static bool isWildcard(std::string_view domain);

        /**
         * @brief Validates domain character set compliance.
         *
         * @details Verifies that domain contains only valid DNS characters
         *          according to RFC specifications for domain names.
         *
         * @param domain String view of domain name to validate.
         */
        static void validateDomainCharacters(std::string_view domain);

        /**
         * @brief Splits domain by dots and validates individual labels.
         *
         * @details Parses domain into labels separated by dots and validates
         *          each label for length and format compliance with DNS standards.
         *
         * @param domain String view of domain name to split and validate.
         */
        static void splitByDotAndValidateLabels(std::string_view domain);

        /**
         * @brief Validates individual label length against DNS limits.
         *
         * @details Checks if domain label length meets DNS label size
         *          restrictions to ensure proper network protocol handling.
         *
         * @param label String view of domain label to check.
         * @param domain String view of full domain for error context.
         */
        static void validateLabelLength(std::string_view label, std::string_view domain);

        /**
         * @brief Validates label format and character composition.
         *
         * @details Verifies that domain label follows proper DNS format
         *          including valid characters and hyphen placement rules.
         *
         * @param label String view of domain label to validate.
         * @param domain String view of full domain for error context.
         */
        static void validateLabelFormat(std::string_view label, std::string_view domain);

        static constexpr auto DOT{'.'};  /**< Domain separator character for label parsing */
    }; // FilterFileValidator
} // FilteringDnsResolver::Filter

#endif // FILTER_FILE_VALIDATOR_HPP

/*** end of file FilterFileValidator.hpp ***/
