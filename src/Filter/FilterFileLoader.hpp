/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         FilterFileLoader.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      30.09.2025                                                    *
 * Last edit:    11.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `FilterFileLoader` class, which     *
 *               implements domain filter file loading and processing for      *
 *               the DNS resolver. It combines file preprocessing and          *
 *               validation functionality to load, clean, and prepare domain   *
 *               filter lists from configuration files. The loader handles     *
 *               deduplication and ensures valid domain format for efficient   *
 *               filtering operations in the DNS resolver application.         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileLoader.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `FilterFileLoader` class for domain filter
 *        file loading, processing, and validation functionality.
 */

#ifndef FILTER_FILE_LOADER_HPP
#define FILTER_FILE_LOADER_HPP

#include "Filter/FilterFilePreprocessor.hpp"
#include "Filter/FilterFileValidator.hpp"
#include <string>  // std::string
#include <vector>  // std::vector

namespace FilteringDnsResolver::Filter
{
    /**
     * @class FilterFileLoader
     * @brief File loader for domain filter configuration processing.
     *
     * @details Combines preprocessing and validation capabilities to load
     *          domain filter files. Handles file parsing, format validation,
     *          and domain list preparation for the DNS filtering engine.
     *          Inherits from FilterFilePreprocessor and FilterFileValidator
     *          for comprehensive file processing functionality.
     */
    class FilterFileLoader final : FilterFilePreprocessor, FilterFileValidator {
    public:
        /**
         * @brief Loads and processes domain filter from configuration file.
         *
         * @details Reads domain names and wildcard patterns from the specified
         *          file, validates each entry, removes duplicates, and populates
         *          the provided vectors. Supports both exact domain matching
         *          and wildcard patterns (*.example.com).
         *
         * @param filterFilePath Path to the filter file containing domain names and wildcard patterns.
         * @param exactDomains Vector to be populated with exact domain names.
         * @param wildcardPatterns Vector to be populated with wildcard patterns.
         *
         * @throws InvalidArgumentException If file cannot be opened or read.
         * @throws InvalidFilterFileContentException If domain format is invalid.
         */
        static void loadFilter(const std::string &filterFilePath,
                               std::vector<std::string> &exactDomains,
                               std::vector<std::string> &wildcardPatterns);

    private:
        /**
         * @brief Removes duplicate domain entries from the filter list.
         *
         * @details Processes the domain list to eliminate duplicate entries,
         *          ensuring each domain appears only once in the final filter
         *          configuration for optimal memory usage and performance.
         *
         * @param filterDomainList Reference to domain list for in-place deduplication.
         */
        static void deduplicateDomains(std::vector<std::string> &filterDomainList);
    }; // FilterFileLoader
} // FilteringDnsResolver::Filter

#endif // FILTER_FILE_LOADER_HPP

/*** end of file FilterFileLoader.hpp ***/
