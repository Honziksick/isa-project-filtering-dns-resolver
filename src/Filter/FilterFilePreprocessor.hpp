/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFilePreprocessor.hpp                                    *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFilePreprocessor.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef FILTER_FILE_PREPROCESSOR_HPP
#define FILTER_FILE_PREPROCESSOR_HPP

#include <string_view>  // std::string_view
#include <string>       // std::string

namespace FilteringDNSResolver::Filter
{
    class FilterFilePreprocessor {
    protected:
        static bool preprocessLine(std::string &line) noexcept;

    private:
        static void trimWhitespace(std::string &line) noexcept;

        static void trimTrailingDot(std::string &line) noexcept;

        static void toLower(std::string &line) noexcept;

        static bool isBlankLine(std::string_view line) noexcept;

        static bool isCommentLine(std::string_view line) noexcept;
    }; // FilterFilePreprocessor
} // FilteringDNSResolver::Filter

#endif // FILTER_FILE_PREPROCESSOR_HPP

/*** end of file FilterFilePreprocessor.hpp ***/
