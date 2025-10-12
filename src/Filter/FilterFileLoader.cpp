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
    void FilterFileLoader::loadFilter(const string &filterFilePath,
                                      vector<string> &exactDomains,
                                      vector<string> &wildcardPatterns) {
        logger("FilterFileLoader::loadFilter() called with file path: '%s'", filterFilePath.c_str());
        verbose("Loading domain filter from file: %s", filterFilePath.c_str());

        // Conversion of std::string to input file stream
        ifstream inputStream{filterFilePath};
        logger("Created input file stream for: '%s'", filterFilePath.c_str());

        // Check if the file was opened successfully
        if(!inputStream.is_open()) {
            logger("ERROR: Failed to open filter file: '%s'", filterFilePath.c_str());
            throw InvalidArgumentException(
                    "Filter file not found or is unreadable: " + filterFilePath
                    );
        } // if

        logger("Filter file successfully opened: '%s'", filterFilePath.c_str());

        string line{};  // Temporary variable to hold each line
        size_t lineNumber = 0;
        size_t processedExactDomains = 0;
        size_t processedWildcards = 0;
        size_t skippedLines = 0;

        logger("Starting line-by-line processing of filter file");

        // Read and process the file line by line
        while(getline(inputStream, line)) {
            lineNumber++;
            logger("Processing line %zu: '%s' (length=%zu)", lineNumber, line.c_str(), line.length());

            if(preprocessLine(line)) {
                logger("Line %zu passed preprocessing: '%s'", lineNumber, line.c_str());

                const bool isExactDomain = validateLine(line);
                logger("Line %zu passed validation", lineNumber);

                if(isExactDomain) {
                    logger("Line %zu is exact domain, adding to exact domains list", lineNumber);
                    exactDomains.emplace_back(move(line));
                    processedExactDomains++;
                } else {
                    logger("Line %zu is wildcard pattern, adding to wildcards list", lineNumber);
                    wildcardPatterns.emplace_back(move(line));
                    processedWildcards++;
                }
            }
            else {
                logger("Line %zu skipped after preprocessing (empty/comment/invalid)", lineNumber);
                skippedLines++;
            }
        } // while

        logger("File processing completed: %zu total lines, %zu exact domains, %zu wildcards, %zu skipped",
               lineNumber, processedExactDomains, processedWildcards, skippedLines);
        verbose("Processed %zu exact domains and %zu wildcard patterns (%zu lines skipped)",
                processedExactDomains, processedWildcards, skippedLines);

        // We remove duplicate domains from the list to optimize filtering
        logger("Deduplicating exact domains (%zu entries)", exactDomains.size());
        deduplicateDomains(exactDomains);
        logger("Exact domains after deduplication: %zu entries", exactDomains.size());

        logger("Deduplicating wildcard patterns (%zu entries)", wildcardPatterns.size());
        deduplicateDomains(wildcardPatterns);
        logger("Wildcard patterns after deduplication: %zu entries", wildcardPatterns.size());

        const size_t totalEntries = exactDomains.size() + wildcardPatterns.size();
        logger("Total entries after deduplication: %zu (%zu exact domains + %zu wildcards)",
               totalEntries, exactDomains.size(), wildcardPatterns.size());
        logger("FilterFileLoader::loadFilter() completed successfully for file: '%s'", filterFilePath.c_str());
        verbose("Filter loaded: %zu exact domains and %zu wildcard patterns ready for blocking",
                exactDomains.size(), wildcardPatterns.size());
    } // FilterFileLoader::loadFilter()

    void FilterFileLoader::deduplicateDomains(vector<string> &filterDomainList) {
        const size_t originalSize = filterDomainList.size();
        logger("FilterFileLoader::deduplicateDomains() called with %zu domains", originalSize);

        logger("Sorting domain list for deduplication");
        ranges::sort(filterDomainList);
        logger("Domain list sorted, removing duplicates");

        filterDomainList.erase(ranges::unique(filterDomainList).begin(), filterDomainList.end());

        const size_t finalSize = filterDomainList.size();
        const size_t removedDuplicates = originalSize - finalSize;

        logger("Deduplication completed: removed %zu duplicates (%zu -> %zu domains)",
               removedDuplicates, originalSize, finalSize);

        if(removedDuplicates > 0) {
            verbose("Removed %zu duplicate domains from filter list", removedDuplicates);
        }
    } // FilterFileLoader::deduplicateDomains()
} // FilteringDnsResolver::Filter

/*** end of file FilterFileLoader.cpp ***/
