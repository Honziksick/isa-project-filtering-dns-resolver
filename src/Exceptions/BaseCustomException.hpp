/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         BaseCustomException.hpp                                       *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `BaseCustomException` template      *
 *               class, which serves as a base class for custom exceptions     *
 *               in the Filtering DNS Resolver. It provides structured error   *
 *               handling with typed error codes, descriptive messages, and    *
 *               additional detail information for comprehensive exception     *
 *               reporting and debugging support throughout the application.   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file BaseCustomException.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `BaseCustomException` template class for
 *        structured error handling and custom exception management.
 */

#ifndef BASE_CUSTOM_EXCEPTION_HPP
#define BASE_CUSTOM_EXCEPTION_HPP

#include <type_traits>  // std::is_enum
#include <exception>    // std::exception
#include <utility>      // std::move
#include <string>       // std::string

namespace FilteringDnsResolver::Exceptions
{
    /**
     * @class BaseCustomException
     * @brief Template base class for custom exceptions with typed error codes.
     *
     * @details Provides structured exception handling with enumerated error codes,
     *          descriptive messages, and additional detail information. Serves as
     *          foundation for domain-specific exception classes throughout the
     *          DNS resolver application with type-safe error code management.
     *
     * @tparam EnumType Enumeration type defining specific error codes for the
     *                  exception domain.
     */
    template <typename EnumType>
    class BaseCustomException : public std::exception {
        static_assert(std::is_enum_v<EnumType>, "EnumType must be an enum type");

    public:
        /**
         * @brief Constructs exception with error code, message, and details.
         *
         * @details Creates a structured exception with all essential error information.
         *          Message and detail strings are moved for efficiency.
         *
         * @param code Typed error code identifying the specific error condition.
         * @param message Primary error message describing the problem.
         * @param detail Additional context and debugging information.
         */
        BaseCustomException(EnumType code, std::string message, std::string detail) noexcept
            : mCode{code},
              mMessage{move(message)},
              mDetail{move(detail)} {}

        /**
         * @brief Returns the primary error message for standard exception handling.
         *
         * @details Provides the main error description as required by std::exception
         *          interface for compatibility with standard exception handling.
         *
         * @return Primary error message as null-terminated string.
         */
        [[nodiscard]]
        const char *what() const noexcept override {
            return mMessage.c_str();
        } // BaseCustomException::what

        /**
         * @brief Returns the error code as integer for numeric comparison.
         *
         * @details Converts the typed error code to integer representation
         *          for logging, comparison, and integration with numeric
         *          error handling systems.
         *
         * @return Error code value as integer.
         */
        [[nodiscard]]
        int code() const noexcept {
            return static_cast<int>(mCode);
        } // BaseCustomException::code

        /**
         * @brief Returns additional error context and debugging information.
         *
         * @details Provides supplementary details about the error condition
         *          for enhanced debugging and troubleshooting capabilities.
         *
         * @return Detailed error information as string.
         */
        [[nodiscard]]
        std::string detail() const noexcept {
            return mDetail;
        } // BaseCustomException::detail

    protected:
        const EnumType mCode;        /**< Typed error code identifying the specific error condition */
        const std::string mMessage;  /**< Primary error message describing the problem */
        std::string mDetail;         /**< Additional context and debugging information */
    }; // BaseCustomException
} // FilteringDnsResolver::Exceptions

#endif // BASE_CUSTOM_EXCEPTION_HPP

/*** end of file BaseCustomException.hpp ***/
