/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         CustomExceptions.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  Header file for the `CustomExceptions` classes used in the    *
 *               Filtering DNS Resolver.                                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file CustomExceptions.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file for the `CustomExceptions` classes.
 */

#ifndef CUSTOM_EXCEPTIONS_HPP
#define CUSTOM_EXCEPTIONS_HPP

#include "Exceptions/BaseCustomException.hpp"
#include <string>  // std::string

namespace FilteringDNSResolver::Exceptions
{
    /**
     * @class HelpRequestedException
     * @brief Exception class used when user requests help.
     */
    class HelpRequestedException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `HelpRequestedException`.
         */
        explicit HelpRequestedException() noexcept;
    }; // HelpRequestedException

    /**
     * @class InternalErrorException
     * @brief Exception class for internal errors.
     */
    class InternalErrorException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `InternalErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit InternalErrorException(std::string detail = "") noexcept;
    }; // InternalErrorException

    /**
     * @class InvalidArgumentException
     * @brief Exception class used when user passes invalid argument to the
     *        program.
     */
    class InvalidArgumentException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `InvalidArgumentException`.
         *
         * @param detail Additional information about the error.
         */
        explicit InvalidArgumentException(std::string detail = "") noexcept;
    }; // InvalidArgumentException

    /**
     * @class UknownErrorException
     * @brief Exception class for unknown errors.
     */
    class UknownErrorException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `UknownErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit UknownErrorException(std::string detail = "") noexcept;
    }; // UknownErrorException

    /**
     * @class InvalidFilterFileContentException
     * @brief Exception class used when domain in invalid format is detected in
     *        the filter file.
     */
    class InvalidFilterFileContentException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `InvalidFilterFileContentException`.
         *
         * @param detail Additional information about the error.
         */
        explicit InvalidFilterFileContentException(std::string detail = "") noexcept;
    }; // InvalidFilterFileContentException

    /**
     * @class ProtocolErrorException
     * @brief Exception class for protocol errors.
     */
    class ProtocolErrorException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `ProtocolErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit ProtocolErrorException(std::string detail = "") noexcept;
    }; // ProtocolErrorException

    /**
     * @class HostnameResolutionErrorException
     * @brief Exception class for hostname resolution errors.
     */
    class HostnameResolutionErrorException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `HostnameResolutionException`.
         *
         * @param detail Additional information about the error.
         */
        explicit HostnameResolutionErrorException(std::string detail = "") noexcept;
    }; // HostnameResolutionErrorException

    /**
     * @class UserInterruptionException
     * @brief Exception class for user interruptions.
     */
    class UserInterruptionException final : public BaseCustomException {
    public:
        /**
         * @brief Constructor for `UserInterruptionException`.
         *
         * @param detail Additional information about the error.
         */
        explicit UserInterruptionException(std::string detail = "") noexcept;
    }; // UserInterruptionException
} // FilteringDNSResolver::Exceptions

#endif // CUSTOM_EXCEPTIONS_HPP

/*** end of file CustomExceptions.hpp ***/
