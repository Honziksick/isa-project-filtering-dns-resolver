/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         StringUtils.hpp                                               *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This file contains the declaration of the `StringUtils`       *
 *               class, which provides utility functions for string            *
 *               operations (e.g., toLower, trimWhitespace, ...).              *
 *                                                                             *
 ******************************************************************************/
/**
 * @file StringUtils.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file defining the `StringUtils` class for string operations.
 */

#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>  // std::string

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class StringUtils
     * @brief Utility class for string operations.
     */
    class StringUtils {
    public:
        /**
         * @brief Converts a `std::string` to lowercase.
         *
         * @note Implementation inspired by Stefan Mai: https://stackoverflow.com/a/313990
         *
         * @param[in,out] str The string to be converted.
         */
        static void toLower(std::string &str) noexcept;

        /**
         * @brief Trims leading and trailing whitespace from a `std::string`.
         *
         * @note Implementation inspired by Assistant: https://www.quora.com/How-do-you-trim-white-spaces-with-C
         *
         * @param[in,out] str The string to be converted.
         */
        static void trimWhitespace(std::string &str) noexcept;
    }; // StringUtils
} // FilteringDnsResolver::Utilities

#endif // STRING_UTILS_HPP

/*** end of file StringUtils.hpp ***/
