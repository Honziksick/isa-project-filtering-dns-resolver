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
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file FilterFileLoader.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef FILTER_FILE_LOADER_HPP
#define FILTER_FILE_LOADER_HPP

#include "Filter/FilterFilePreprocessor.hpp"
#include "Filter/FilterFileValidator.hpp"
#include <string>  // std::string
#include <vector>  // std::vector

namespace FilteringDNSResolver::Filter
{
    class FilterFileLoader final : FilterFilePreprocessor, FilterFileValidator {
    public:
        static std::vector<std::string> loadFilter(const std::string &filterFilePath);
    }; // FilterFileLoader
} // FilteringDNSResolver::Filter

#endif // FILTER_FILE_LOADER_HPP

/*** end of file FilterFileLoader.hpp ***/
