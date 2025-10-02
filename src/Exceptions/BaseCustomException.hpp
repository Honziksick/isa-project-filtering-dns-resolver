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
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Header file for the `BaseCustomException` class used in the   *
 *               Filtering DNS Resolver.                                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file BaseCustomException.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring the `BaseCustomException` class used in the
 *        Filtering DNS Resolver.
 */

#ifndef BASE_CUSTOM_EXCEPTION_HPP
#define BASE_CUSTOM_EXCEPTION_HPP

#include "Enums/ExitCodes.hpp"
#include <exception> // std::exception
#include <string>    // std::string

namespace FilteringDnsResolver::Exceptions
{
    /**
     * @class BaseCustomException
     * @brief Exception class for handling errors in the Filtering DNS Resolver.
     */
    class BaseCustomException : public std::exception {
    public:
        /**
         * @brief Constructor for BaseCustomException.
         * @param code The error code.
         * @param message The error message.
         * @param detail Additional details about the error.
         */
        BaseCustomException(Enums::ExitCodes code, std::string message, std::string detail) noexcept;

        /**
         * @brief Returns the error message.
         * @return The error message as a C-style string.
         */
        [[nodiscard]]
        const char *what() const noexcept override;

        /**
         * @brief Returns the error code as integer.
         * @return The error code value.
         */
        [[nodiscard]]
        int code() const noexcept;

        /**
         * @brief Returns additional details about the error.
         * @return The error details as a string.
         */
        [[nodiscard]]
        std::string detail() const noexcept;

    protected:
        const Enums::ExitCodes mCode;  /**< The error code.    */
        const std::string mMessage;    /**< The error message. */
        std::string mDetail;           /**< Additional details about the error. */
    }; // BaseCustomException
} // FilteringDnsResolver::Exceptions

#endif // BASE_CUSTOM_EXCEPTION_HPP

/*** end of file BaseCustomException.hpp ***/
