/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFileLoader.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileLoader.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "Filter/FilterFileLoader.hpp"
#include "Filter/FilterFilePreprocessor.hpp"
#include "Filter/FilterFileValidator.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <algorithm>  // std::transform
#include <fstream>    // std::ifstream
#include <ranges>     // std::ranges::find_if_not, std::ranges::sort, std::ranges::unique
#include <cctype>     // std::isspace
#include <vector>     // std::vector
#include <string>     // std::string

using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::Filter
{
    vector<string> FilterFileLoader::loadFilter(const string &filterFilePath) {
        logger("Loading filter file lines: '%s'", filterFilePath.c_str());

        // Conversion of std::string to input file stream
        ifstream inputStream{filterFilePath};

        // Check if the file was opened successfully
        if(!inputStream.is_open()) {
            throw InvalidArgumentException(
                    "Filter file not found or is unreadable: " + filterFilePath
                    );
        }

        vector<string> filterDomainList;  // Store the raw lines from the file
        string line;  // Temporary variable to hold each line

        // Read and process the file line by line
        while(getline(inputStream, line)) {
            if(preprocessLine(line)) {
                validateLine(line);
                filterDomainList.emplace_back(move(line));
            }
        }

        // We remove duplicate domains from the list to optimize filtering
        deduplicateDomains(filterDomainList);

        return filterDomainList;
    } // FilterFileLoader::loadFilter()

    void FilterFileLoader::deduplicateDomains(vector<string> &filterDomainList) {
        ranges::sort(filterDomainList);
        filterDomainList.erase(ranges::unique(filterDomainList).begin(), filterDomainList.end());
    } // FilterFileLoader::deduplicateDomains()
} // FilteringDnsResolver::Filter

/*** end of file FilterFileLoader.cpp ***/
