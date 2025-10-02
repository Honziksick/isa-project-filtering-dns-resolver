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
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileValidator.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef FILTER_FILE_VALIDATOR_HPP
#define FILTER_FILE_VALIDATOR_HPP

#include <string_view>  // std::string_view

namespace FilteringDnsResolver::Filter
{
    class FilterFileValidator {
    protected:
        static void validateLine(std::string_view line);

    private:
        static void validateDomainLength(std::string_view domain);
        static void validateDomainCharacters(std::string_view domain);
        static void splitByDotAndValidateLabels(std::string_view domain);
        static void validateLabelLength(std::string_view label, std::string_view domain);
        static void validateLabelFormat(std::string_view label, std::string_view domain);

        static constexpr auto DOT{'.'};
    }; // FilterFileValidator
} // FilteringDnsResolver::Filter

#endif // FILTER_FILE_VALIDATOR_HPP

/*** end of file FilterFileValidator.hpp ***/
