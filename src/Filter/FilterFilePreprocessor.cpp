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
#include "Utilities/StringUtils.hpp"
#include <string_view>  // std::views::reverse
#include <string>       // std::string

using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::Filter
{
    bool FilterFilePreprocessor::preprocessLine(string &line) noexcept {
        StringUtils::trimWhitespace(line);
        if(isBlankLine(line) || isCommentLine(line)) {
            return false;
        }
        trimTrailingDot(line);
        StringUtils::toLower(line);

        return true;
    } // FilterFilePreprocessor::preprocessLine()

    void FilterFilePreprocessor::trimTrailingDot(string &line) noexcept {
        if(line.back() == '.') {
            line.pop_back();
        }
    } // FilterFilePreprocessor::trimTrailingDot()

    bool FilterFilePreprocessor::isBlankLine(const string_view line) noexcept {
        return line.empty();
    } // FilterFilePreprocessor::isBlankLine()

    bool FilterFilePreprocessor::isCommentLine(const string_view line) noexcept {
        return line.front() == '#';
    } // FilterFilePreprocessor::isCommentLine()
} // FilteringDnsResolver::Filter

/*** end of file FilterFilePreprocessor.cpp ***/
