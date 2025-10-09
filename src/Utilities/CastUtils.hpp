/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         CastUtils.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This file contains the declaration of the `CastUtils`         *
 *               class, which provides utility methodss for type casting,      *
 *               specifically for enums. It includes methods to convert        *
 *               enums to their integer or string representations.             *
 *                                                                             *
 ******************************************************************************/
/**
 * @file CastUtils.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Declarations of the `CastUtils` class for type casting utilities.
 */

#ifndef CAST_UTILS_HPP
#define CAST_UTILS_HPP

#include "Enums/Mapping/EnumMappers.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <type_traits>  // std::is_enum_v, std::is_integral_v, std::is_same_v
#include <algorithm>    // std::ranges::clamp
#include <cstdint>      // uint8_t, uint16_t
#include <limits>       // std::numeric_limits
#include <chrono>       // std::chrono
#include <string>       // std::string
#include <vector>       // std::vector

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class CastUtils
     * @brief A utility class providing template helper methods for type casting.
     */
    class CastUtils {
    public:
        /**
         * @brief Converts an integer value to its `size_t` representation.
         * @details This function uses a static assertion to ensure that the
         *          template parameter is an integral type. If a non-integral
         *          type is used, a compile-time error will occur. The function
         *          then safely casts the integer value to `size_t`.
         *
         * @tparam IntegerType The type of the integer to be converted.
         *                     Must be an integral type.
         * @param integerValue The integer value to convert.
         *
         * @return The `size_t` representation of the integer value.
         */
        template <typename IntegerType>
        static constexpr size_t castIntToSizeT(IntegerType integerValue) {
            static_assert(std::is_integral_v<IntegerType>, "Template parameter must be an integral type");
            return static_cast<size_t>(integerValue);
        } // CastUtils::castIntToSizeT

        /**
         * @brief Combines two consecutive bytes from a vector into a 16-bit word.
         * @details This function takes two consecutive bytes from the specified
         *          starting index in the vector, combines them into a 16-bit word.
         *          The imput vector should contain bytes in network byte order
         *          (big-endian order). Which means that the first byte is the
         *          most significant byte.
         *
         * @note Example: If the input vector contains the bytes {0x12, 0x34},
         *       and the starting index is 0, the resulting word will be 0x1234.
         *
         * @tparam IntegerType The type of the elements in the input vector.
         *                     Must be `uint8_t`.
         *
         * @param byteVector The vector containing the bytes to be combined.
         * @param startIndex The starting index in the vector for the two bytes
         *                   to be combined.
         *
         * @return uint16_t The big-endian 16-bit word created from the two bytes.
         */
        template <typename IntegerType>
        static constexpr uint16_t castTwoBytesToWord(const std::vector<IntegerType> &byteVector, size_t startIndex) {
            static_assert(std::is_same_v<IntegerType, uint8_t>, "Template parameter must be uint8_t.");

            // Check if the startIndex and the next byte are within the bounds of the vector
            if(startIndex + 1 >= byteVector.size()) {
                throw Exceptions::InternalErrorException(
                        "Index out of range for castBytesToWord. Index: " + std::to_string(startIndex) +
                        ", Vector size: " + std::to_string(byteVector.size())
                        );
            }

            // Extract the bytes
            const auto higherByte = static_cast<uint16_t>(byteVector[startIndex] << 8);
            const auto lowerByte = static_cast<uint16_t>(byteVector[startIndex + 1]);

            //return higherByte | lowerByte;
            return higherByte | lowerByte;
        } // CastUtils::castTwoBytesToWord

        /**
         * @brief Splits a 16-bit word into two bytes and returns them as a vector.
         * @details This function takes a 16-bit word and splits it into its higher
         *          and lower bytes. The resulting bytes are returned in a vector
         *          in network byte order (big-endian), where the first byte is the
         *          most significant byte.
         *
         * @note Example: If the input word is 0x1234, the resulting vector will
         *       contain the bytes {0x12, 0x34}.
         *
         * @tparam IntegerType The type of the word to be converted. Must be `uint16_t`.
         *
         * @param word The 16-bit word to be split into two bytes.
         *
         * @return std::vector<uint8_t> A vector containing the two bytes in big-endian order.
         */
        template <typename IntegerType>
        static constexpr std::vector<uint8_t> castWordToTwoBytes(const uint16_t word) {
            static_assert(std::is_same_v<IntegerType, uint16_t>, "Template parameter must be uint16_t.");

            // Extract the bytes
            const auto higherByte = static_cast<uint8_t>(word >> 8);
            const auto lowerByte = static_cast<uint8_t>(word & 0x00FF);

            return std::vector<uint8_t>{higherByte, lowerByte};
        } // CastUtils::castWordToTwoBytes

        /**
         * @brief Converts a raw byte array to a `std::vector<uint8_t>`.
         * @details Creates a new vector and copies the specified number of bytes
         *          from the provided raw pointer. This function is safe to call
         *          with `length == 0` and a `NULL` pointer - in that case it
         *          returns an empty vector. If the pointer is `NULL` and
         *          `length > 0`, an exception is thrown.
         *
         * @param pData Pointer to the source byte array (may be `nullptr` only
         *              when `length == 0`).
         * @param length Number of bytes to copy from the source array.
         *
         * @return std::vector<uint8_t> A vector containing a copy of the input bytes.
         *
         * @throws Exceptions::InternalErrorException If `data` is `nullptr` while `length > 0`.
         */
        static std::vector<uint8_t> castByteArrayToVector(const uint8_t *pData, const size_t length) {
            if(length == 0) {
                return {};
            }

            if(pData == nullptr) {
                throw Exceptions::InternalErrorException(
                        "Null data pointer with non-zero length was given to `castByteArrayToVector`"
                        );
            }

            std::vector<uint8_t> out(length);
            for(size_t iByte = 0; iByte < length; iByte++) {
                out[iByte] = pData[iByte];
            }
            return out;
        } // CastUtils::castByteArrayToVector

        /**
         * @brief Copies bytes from a `std::vector<uint8_t>` into a raw byte array.
         * @details Copies up to `destinationSize` bytes from `source` into
         *          `pDestination`. If `destinationSize` is smaller than
         *          `source.size()`, the copy is truncated to fit the destination.
         *          It is safe to call this function with `destinationSize == 0`
         *          and a `nullptr` - in that case, nothing is written and 0 is
         *          returned.
         *
         * @param source Source vector containing the bytes to copy.
         * @param pDestination Destination pointer to a writable byte array
         *                     (may be `nullptr` only when `destinationSize == 0`).
         * @param destinationSize Capacity of the destination array in bytes.
         *
         * @return size_t The number of bytes written to `pDestination`.
         *
         * @throws Exceptions::InternalErrorException If `pDestination` is `nullptr` while `destSize > 0`.
         */
        static size_t castVectorToByteArray(const std::vector<uint8_t> &source,
                                            uint8_t *pDestination, const size_t destinationSize) {
            if(destinationSize == 0) {
                return 0;
            }

            if(pDestination == nullptr) {
                throw Exceptions::InternalErrorException(
                        "Null destination pointer with non-zero size in castVectorToByteArray");
            }

            const size_t toCopy = (source.size() < destinationSize) ? source.size() : destinationSize;
            for(size_t iByte = 0; iByte < toCopy; iByte++) {
                pDestination[iByte] = source[iByte];
            }
            return toCopy;
        } // CastUtils::castVectorToByteArray

        /**
         * @brief Converts an integer value to its corresponding enum representation.
         * @details This function uses a static assertion to ensure that the
         *          template parameter is an enum type. If a non-enum type is
         *          used, a compile-time error will occur. The function then
         *          safely casts the integer value to the specified enum type.
         *
         * @param integerValue The integer value to convert to enum.
         * @tparam EnumType The type of the enum to be converted to. Must be an enum type.
         *
         * @return EnumType The enum representation of the integer value.
         */
        template <typename EnumType>
        static constexpr EnumType castIntToEnum(int integerValue) {
            static_assert(std::is_enum_v<EnumType>, "Template parameter must be an EnumType");
            return static_cast<EnumType>(integerValue);
        } // CastUtils::castIntToEnum

        /**
         * @brief Converts an enum value to its underlying integer representation.
         * @details This function uses a static assertion to ensure that the
         *          template parameter is an enum type. If a non-enum type is
         *          used, a compile-time error will occur.
         *
         * @tparam EnumType The type of the enum to be converted. Must be an enum type.
         * @param enumValue The enum value to convert.
         *
         * @return int The integer representation of the enum value.
         */
        template <typename EnumType>
        static constexpr int castEnumToInt(EnumType enumValue) {
            static_assert(std::is_enum_v<EnumType>, "Template parameter must be an enum type");
            return static_cast<int>(enumValue);
        } // CastUtils::castEnumToInt

        /**
         * @brief Converts an enum value to its `uint8_t` representation.
         * @details This function uses a static assertion to ensure that the
         *          template parameter is an enum type. If a non-enum type is
         *          used, a compile-time error will occur.
         *
         * @tparam EnumType The type of the enum to be converted. Must be an enum type.
         * @param enumValue The enum value to convert.
         *
         * @return uint8_t The unsigned short representation of the enum value.
         */
        template <typename EnumType>
        static constexpr int castEnumToByte(EnumType enumValue) {
            static_assert(std::is_enum_v<EnumType>, "Template parameter must be an enum type");
            return static_cast<uint8_t>(enumValue);
        } // CastUtils::castEnumToByte


        /**
         * @brief Converts an enum value to its string representation.
         * @details This method uses a static assertion to ensure that the
         *          template parameter is an enum type. It retrieves the
         *          corresponding map for the enum type using `EnumMaps::getEnumToStringMap`.
         *          If the value is found in the map, it returns the corresponding
         *          string. Otherwise, it throws an exception indicating that
         *          the value is not present in the map.
         *
         * @tparam EnumType The type of the enum to be converted. Must be an enum type.
         * @param enumValue The enum value to convert.
         *
         * @return std::string The string representation of the enum value.
         *
         * @note This function was co-created with the help of GitHub Copilot.
         */
        template <typename EnumType>
        static std::string castEnumToString(EnumType enumValue) {
            static_assert(std::is_enum_v<EnumType>, "Template parameter must be an enum type");

            // Retrieve the map for the given enum type
            const auto &enumToStringMap = Enums::Mapping::EnumMappers::getEnumToStringMap<EnumType>();

            // Find the string value in the map
            for(const auto &pair : enumToStringMap) {
                if(pair.first == enumValue) {
                    return pair.second;
                }
            }

            // Throw an exception if the value is not found
            throw Exceptions::InternalErrorException(
                    "Enum value not found in the map. Enum type: " + std::string(typeid(EnumType).name()) +
                    ", value: " + std::to_string(static_cast<int>(enumValue))
                    );
        } // CastUtils::castEnumToString

        /**
         * @brief Converts a string to its corresponding enum value.
         * @details This method retrieves the map of string-to-enum mappings
         *          for the given enum type and finds the enum value corresponding
         *          to the provided string. If the string is not found,
         *          an exception is thrown.
         *
         * @note My implementation was inspired by the previous method.
         *
         * @tparam EnumType The type of the enum to be converted. Must be an enum type.
         * @param strValue The string representation of the enum value.
         *
         * @return EnumType The enum value corresponding to the string.
         */
        template <typename EnumType>
        static EnumType castStringToEnum(const std::string &strValue) {
            static_assert(std::is_enum_v<EnumType>, "Template parameter must be an enum type");

            // Retrieve the map for the given enum type
            const auto &enumToStringMap = Enums::Mapping::EnumMappers::getEnumToStringMap<EnumType>();

            // Find the enum value in the map
            for(const auto &pair : enumToStringMap) {
                if(pair.second == strValue) {
                    return pair.first;
                }
            }

            // Throw an exception if the string is not found
            return EnumType::ANY;
        } // CastUtils::castStringToEnum

        static int castMillisecondsToInt(const std::chrono::milliseconds ms) {
            const auto clamped = std::ranges::clamp(
                    static_cast<long long>(ms.count()),
                    static_cast<long long>(std::numeric_limits<int>::min()),
                    static_cast<long long>(std::numeric_limits<int>::max())
                    );
            return static_cast<int>(clamped);
        } // CastUtils::castMillisecondsToInt
    }; // CastUtils
} // FilteringDnsResolver::Utilities

#endif // CAST_UTILS_HPP

/*** end of file CastUtils.hpp ***/
