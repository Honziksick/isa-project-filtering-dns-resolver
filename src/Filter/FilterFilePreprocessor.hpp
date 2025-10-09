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
 * Description:  This header file provides `FilterFilePreprocessor` class,     *
 *               which implements preprocessing functionality for domain       *
 *               filter configuration files. It provides line-by-line          *
 *               processing to clean and normalize domain entries, handle      *
 *               comments and blank lines, and prepare raw file content for    *
 *               validation and filtering engine consumption. The preprocessor *
 *               ensures consistent domain format and removes unwanted content *
 *               from filter configuration files.                              *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFilePreprocessor.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `FilterFilePreprocessor` class for domain
 *        filter file preprocessing and line normalization functionality.
 */

#ifndef FILTER_FILE_PREPROCESSOR_HPP
#define FILTER_FILE_PREPROCESSOR_HPP

#include <string_view>  // std::string_view
#include <string>       // std::string

namespace FilteringDnsResolver::Filter
{
    /**
     * @class FilterFilePreprocessor
     * @brief Base class for domain filter file preprocessing and normalization.
     *
     * @details Provides line-by-line preprocessing functionality for domain filter
     *          configuration files. Handles comment removal, blank line detection,
     *          and domain format normalization to prepare raw file content for
     *          validation and filtering engine consumption.
     */
    class FilterFilePreprocessor {
    protected:
        /**
         * @brief Preprocesses a single line from the filter configuration file.
         *
         * @details Normalizes domain format, removes trailing dots, and filters
         *          out comments and blank lines. Modifies the input line in-place
         *          and indicates whether the line should be processed further.
         *
         * @param line Reference to line string for in-place modification.
         *
         * @return `true` if line contains valid domain data, `false` if line
         *          should be skipped.
         */
        static bool preprocessLine(std::string &line) noexcept;

    private:
        /**
         * @brief Removes trailing dot from domain name for normalization.
         *
         * @details Strips the trailing dot character from fully qualified domain
         *          names to ensure consistent domain format throughout the filter.
         *
         * @param line Reference to line string for in-place dot removal.
         */
        static void trimTrailingDot(std::string &line) noexcept;

        /**
         * @brief Checks if line contains only whitespace characters.
         *
         * @details Determines whether the line is empty or contains only
         *          whitespace characters that should be skipped during processing.
         *
         * @param line String view of the line to check.
         *
         * @return `true` if line is blank, `false` otherwise.
         */
        static bool isBlankLine(std::string_view line) noexcept;

        /**
         * @brief Checks if line starts with comment character.
         *
         * @details Identifies comment lines that should be excluded from
         *          domain processing based on comment prefix characters.
         *
         * @param line String view of the line to check.
         *
         * @return `true` if line is a comment, `false` otherwise.
         */
        static bool isCommentLine(std::string_view line) noexcept;
    }; // FilterFilePreprocessor
} // FilteringDnsResolver::Filter

#endif // FILTER_FILE_PREPROCESSOR_HPP

/*** end of file FilterFilePreprocessor.hpp ***/
