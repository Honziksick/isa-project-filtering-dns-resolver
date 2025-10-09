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
 * Description:  This source file implements the `FilterFilePreprocessor`      *
 *               class, which implements preprocessing functionality for       *
 *               domain filter configuration files. It provides line-by-line   *
 *               processing to clean and normalize domain entries, handle      *
 *               comments and blank lines, and prepare raw file content for    *
 *               validation and filtering engine consumption. The preprocessor *
 *               ensures consistent domain format and removes unwanted content *
 *               from filter configuration files.                              *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFilePreprocessor.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `FilterFilePreprocessor` class for domain
 *        filter file preprocessing and line normalization functionality.
 */

#include "Filter/FilterFilePreprocessor.hpp"
#include "Utilities/StringUtils.hpp"
#include "Utilities/Logger.hpp"
#include <string_view>  // std::views::reverse
#include <string>       // std::string

using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::Filter
{
    bool FilterFilePreprocessor::preprocessLine(string &line) noexcept {
        const string originalLine = line;  // Store original for logging
        logger("FilterFilePreprocessor::preprocessLine() called with line: '%s' (length=%zu)",
               originalLine.c_str(), originalLine.length());

        logger("Step 1: Trimming whitespace from line");
        StringUtils::trimWhitespace(line);
        if(line != originalLine) {
            logger("Whitespace trimmed: '%s' -> '%s'", originalLine.c_str(), line.c_str());
        }
        else {
            logger("No whitespace to trim");
        }

        logger("Step 2: Checking if line is blank or comment");
        if(isBlankLine(line)) {
            logger("Line is blank after trimming - rejecting");
            return false;
        }
        if(isCommentLine(line)) {
            logger("Line is comment (starts with '#') - rejecting");
            return false;
        }
        logger("Line passed blank/comment check");

        logger("Step 3: Trimming trailing dot");
        const bool hadTrailingDot = !line.empty() && line.back() == '.';
        trimTrailingDot(line);
        if(hadTrailingDot) {
            logger("Trailing dot removed: '%s'", line.c_str());
        }
        else {
            logger("No trailing dot to remove");
        }

        logger("Step 4: Converting to lowercase");
        const string beforeLowercase = line;
        StringUtils::toLower(line);
        if(line != beforeLowercase) {
            logger("Converted to lowercase: '%s' -> '%s'", beforeLowercase.c_str(), line.c_str());
        }
        else {
            logger("Already lowercase, no changes needed");
        }

        logger("Preprocessing completed successfully: '%s' -> '%s'", originalLine.c_str(), line.c_str());
        return true;
    } // FilterFilePreprocessor::preprocessLine

    void FilterFilePreprocessor::trimTrailingDot(string &line) noexcept {
        logger("FilterFilePreprocessor::trimTrailingDot() called with line: '%s'", line.c_str());

        if(line.empty()) {
            logger("Line is empty - no trailing dot to trim");
            return;
        }

        if(line.back() == '.') {
            logger("Trailing dot found - removing from line");
            line.pop_back();
            logger("Trailing dot removed, result: '%s'", line.c_str());
        }
        else {
            logger("No trailing dot found (last char: '%c')", line.back());
        }
    } // FilterFilePreprocessor::trimTrailingDot

    bool FilterFilePreprocessor::isBlankLine(const string_view line) noexcept {
        const bool blank = line.empty();
        logger("FilterFilePreprocessor::isBlankLine() called: line length=%zu, result=%s",
               line.length(), blank ? "BLANK" : "NOT_BLANK");
        return blank;
    } // FilterFilePreprocessor::isBlankLine

    bool FilterFilePreprocessor::isCommentLine(const string_view line) noexcept {
        logger("FilterFilePreprocessor::isCommentLine() called with line: '%.*s' (length=%zu)",
               static_cast<int>(line.length()), line.data(), line.length());

        if(line.empty()) {
            logger("Line is empty - not a comment");
            return false;
        }

        const bool isComment = line.front() == '#';
        logger("First character: '%c', result=%s", line.front(), isComment ? "COMMENT" : "NOT_COMMENT");
        return isComment;
    } // FilterFilePreprocessor::isCommentLine
} // FilteringDnsResolver::Filter

/*** end of file FilterFilePreprocessor.cpp ***/
