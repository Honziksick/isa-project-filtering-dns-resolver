/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         EnumMappers.hpp                                               *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Declaration of the `EnumMappers` class, which provides        *
 *               static template mapping methods for converting enum values    *
 *               to their corresponding string representations.                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file EnumMappers.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring static template methods for converting enum
 *        values to their string representations.
 */

#ifndef ENUM_MAPPERS_HPP
#define ENUM_MAPPERS_HPP

#include "Enums/Mapping/EnumMaps.hpp"
#include "Enums/ExitCodes.hpp"
#include "Enums/DnsOpcodes.hpp"
#include "Enums/DnsRCodes.hpp"
#include <unordered_map>  // std::unordered_map
#include <string>         // std::string

namespace FilteringDnsResolver::Enums::Mapping
{
    /**
     * @class EnumMappers
     * @brief Provides static template methods for mapping enum values to their
     *        string representations.
     *
     * @details The `EnumMappers` class extends the `EnumMaps` class and offers
     *          a template method to retrieve mappings of enum values to their
     *          corresponding string representations. Each enum type must have
     *          a specialized implementation of the template method.
     *
     * @note Inspired by: https://www.fit.vut.cz/person/peringer/public/ICP/Prednasky/ICP.pdf (p. 160)
     */
    class EnumMappers : public EnumMaps {
    public:
        /**
         * @brief Template function to retrieve the mapping for a given enum
         *        type to strings.
         *
         * @tparam EnumType The enum type for which the mapping is requested.
         * @return A constant reference to the map of the specified `EnumType`
         *         to strings.
         *
         * @note This method must be specialized for each supported enum type.
         */
        template <typename EnumType>
        static const std::unordered_map<EnumType, std::string> &getEnumToStringMap();
    }; // EnumMappers


    /**
     * @brief Specialization of the template method to retrieve the mapping for `ExitCodes`.
     * @return A constant reference to the map of `ExitCodes` to strings.
     *
     * @note Inspired by: https://www.fit.vut.cz/person/peringer/public/ICP/Prednasky/ICP.pdf
     */
    template <>
    inline const std::unordered_map<ExitCodes, std::string> &EnumMappers::getEnumToStringMap<ExitCodes>() {
        return getExitCodesMap();
    }

    /**
     * @brief Specialization of the template method to retrieve the mapping for `DnsOpcodes`.
     * @return A constant reference to the map of `DnsOpcodes` to strings.
     *
     * @note Inspired by: https://www.fit.vut.cz/person/peringer/public/ICP/Prednasky/ICP.pdf
     */
    template <>
    inline const std::unordered_map<DnsOpcodes, std::string> &EnumMappers::getEnumToStringMap<DnsOpcodes>() {
        return getDnsOpcodesMap();
    }

    /**
     * @brief Specialization of the template method to retrieve the mapping for `DnsRCodes`.
     * @return A constant reference to the map of `DnsRCodes` to strings.
     *
     * @note Inspired by: https://www.fit.vut.cz/person/peringer/public/ICP/Prednasky/ICP.pdf
     */
    template <>
    inline const std::unordered_map<DnsRCodes, std::string> &EnumMappers::getEnumToStringMap<DnsRCodes>() {
        return getDnsRCodesMap();
    }
} // FilteringDnsResolver::Enums::Mapping

#endif // ENUM_MAPPERS_HPP

/*** end of file EnumMappers.hpp ***/
