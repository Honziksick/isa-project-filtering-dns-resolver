/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFilePreprocessor.cpp                                    *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFilePreprocessor.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "Filter/FilterFilePreprocessor.hpp"
#include <string_view>  // std::views::reverse
#include <algorithm>    // std::transform
#include <utility>      // std::move
#include <ranges>       // std::ranges::find_if_not
#include <cctype>       // std::isspace
#include <string>       // std::string

using namespace std;

namespace FilteringDNSResolver::Filter
{
    bool FilterFilePreprocessor::preprocessLine(string &line) noexcept {
        trimWhitespace(line);
        if(isBlankLine(line) || isCommentLine(line)) {
            return false;
        }
        trimTrailingDot(line);
        toLower(line);

        return true;
    } // FilterFilePreprocessor::preprocessLine()

    // Implementation inspired by Assistant: https://www.quora.com/How-do-you-trim-white-spaces-with-C
    void FilterFilePreprocessor::trimWhitespace(string &line) noexcept {
        // Lambda function serving as a predicate for whitespace characters
        auto isWhitespace = [](const unsigned char character) {
            return isspace(character) != 0;
        };

        // Find the first non-whitespace character from the beginning
        const auto leadingWhitespaceIt = ranges::find_if_not(line, isWhitespace);

        // Find the first non-whitespace character from the end (base converts reverse iterator to normal)
        const auto trailingWhitespaceIt = ranges::find_if_not(line | views::reverse, isWhitespace).base();

        // If the entire line is whitespace, clear it
        if(leadingWhitespaceIt >= trailingWhitespaceIt) {
            line.clear();
        }
        // Else create a substring that excludes leading and trailing whitespace
        else {
            line.assign(leadingWhitespaceIt, trailingWhitespaceIt);
        }
    } // FilterFilePreprocessor::trimWhitespace()

    void FilterFilePreprocessor::trimTrailingDot(string &line) noexcept {
        if(line.back() == '.') {
            line.pop_back();
        }
    } // FilterFilePreprocessor::trimTrailingDot()

    // Implementation inspired by Stefan Mai: https://stackoverflow.com/a/313990
    void FilterFilePreprocessor::toLower(string &line) noexcept {
        auto toLowerCharacter = [](const unsigned char character) {
            return tolower(character);
        };

        ranges::transform(line, line.begin(), toLowerCharacter);
    } // FilterFilePreprocessor::toLower

    bool FilterFilePreprocessor::isBlankLine(const string_view line) noexcept {
        return line.empty();
    } // FilterFilePreprocessor::isBlankLine()

    bool FilterFilePreprocessor::isCommentLine(const string_view line) noexcept {
        return line.front() == '#';
    } // FilterFilePreprocessor::isCommentLine()
} // FilteringDNSResolver::Filter

/*** end of file FilterFilePreprocessor.cpp ***/
