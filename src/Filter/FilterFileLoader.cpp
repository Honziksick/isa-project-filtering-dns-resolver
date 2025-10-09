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
 * Description:  This source file implements the `FilterFileLoader` class,     *
 *               which implements domain filter file loading and processing    *
 *               for the DNS resolver. It combines file preprocessing and      *
 *               validation functionality to load, clean, and prepare domain   *
 *               filter lists from configuration files. The loader handles     *
 *               deduplication and ensures valid domain format for efficient   *
 *               filtering operations in the DNS resolver application.         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileLoader.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implements the `FilterFileLoader` class for domain filter
 *        file loading, processing, and validation functionality.
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
