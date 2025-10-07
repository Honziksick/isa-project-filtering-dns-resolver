/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         StringUtils.cpp                                               *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This file contains the implementation of the `StringUtils`    *
 *               class, which provides utility functions for string            *
 *               operations (e.g., toLower, trimWhitespace, ...).              *
 *                                                                             *
 ******************************************************************************/
/**
 * @file StringUtils.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of the `StringUtils` class for string operations.
 */

#include "Utilities/StringUtils.hpp"
#include <algorithm>  // std::transform
#include <string>     // std::string
#include <cctype>     // std::tolower
#include <ranges>     // std::ranges

using namespace std;

namespace FilteringDnsResolver::Utilities
{
    void StringUtils::toLower(string &str) noexcept {
        auto toLowerCharacter = [](const unsigned char character) {
            return tolower(character);
        };

        ranges::transform(str, str.begin(), toLowerCharacter);
    } // StringUtils::toLower

    void StringUtils::trimWhitespace(string &str) noexcept {
        // Lambda function serving as a predicate for whitespace characters
        auto isWhitespace = [](const unsigned char character) {
            return isspace(character) != 0;
        };

        // Find the first non-whitespace character from the beginning
        const auto leadingWhitespaceIt = ranges::find_if_not(str, isWhitespace);

        // Find the first non-whitespace character from the end (base converts reverse iterator to normal)
        const auto trailingWhitespaceIt = ranges::find_if_not(str | views::reverse, isWhitespace).base();

        // If the entire line is whitespace, clear it
        if(leadingWhitespaceIt >= trailingWhitespaceIt) {
            str.clear();
        }
        // Else create a substring that excludes leading and trailing whitespace
        else {
            str.assign(leadingWhitespaceIt, trailingWhitespaceIt);
        }
    } // StringUtils::trimWhitespace()
} // FilteringDnsResolver::Utilities

/*** end of file StringUtils.cpp ***/
