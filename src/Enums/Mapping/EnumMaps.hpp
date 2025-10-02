/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         EnumMaps.hpp                                                  *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Declaration of the `EnumMaps` class, which provides static    *
 *               mapping utilities for converting enum values to their         *
 *               corresponding string representations.                         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file EnumMaps.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring static methods to retrieve enum-to-string maps.
 */

#ifndef ENUM_MAPS_HPP
#define ENUM_MAPS_HPP

#include "Enums/ExitCodes.hpp"
#include <unordered_map>  // std::unordered_map
#include <string>         // std::string

namespace FilteringDnsResolver::Enums::Mapping
{
    /**
     * @class EnumMaps
     * @brief Provides static methods to retrieve enum-to-string mappings.
     *
     * @details The `EnumMaps` class contains static methods that return constant
     *          references to `std::unordered_map` objects. These maps define the
     *          relationships between enum values and their corresponding string
     *          representations. The class is designed to be used as a utility
     *          without requiring instantiation.
     *
     * @note Inspired by: https://www.fit.vut.cz/person/peringer/public/ICP/Prednasky/ICP.pdf (p. 160)
     */
    class EnumMaps {
    protected:
        /**
         * @brief Retrieves the mapping for `ExitCodes` to strings.
         * @return A constant reference to the map of `ExitCodes` to strings.
         */
        static const std::unordered_map<ExitCodes, std::string> &getExitCodesMap();
    }; // EnumMaps
} // FilteringDnsResolver::Enums::Mapping

#endif // ENUM_MAPS_HPP

/*** end of file EnumMaps.hpp ***/
