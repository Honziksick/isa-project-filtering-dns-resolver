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
 * Last edit:    07.10.2025                                                    *
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
#include "Enums/ExitCodes.hpp"
#include "Enums/DnsRCodes.hpp"
#include <string>  // std::string

namespace FilteringDnsResolver::Exceptions
{
    /**
     * @class HelpRequestedException
     * @brief Exception class used when user requests help.
     */
    class HelpRequestedException final : public BaseCustomException<Enums::ExitCodes> {
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
    class InternalErrorException final : public BaseCustomException<Enums::ExitCodes> {
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
    class InvalidArgumentException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `InvalidArgumentException`.
         *
         * @param detail Additional information about the error.
         */
        explicit InvalidArgumentException(std::string detail = "") noexcept;
    }; // InvalidArgumentException

    /**
     * @class UnknownErrorException
     * @brief Exception class for unknown errors.
     */
    class UnknownErrorException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `UnknownErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit UnknownErrorException(std::string detail = "") noexcept;
    }; // UnknownErrorException

    /**
     * @class InvalidFilterFileContentException
     * @brief Exception class used when domain in invalid format is detected in
     *        the filter file.
     */
    class InvalidFilterFileContentException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `InvalidFilterFileContentException`.
         *
         * @param detail Additional information about the error.
         */
        explicit InvalidFilterFileContentException(std::string detail = "") noexcept;
    }; // InvalidFilterFileContentException

    /**
     * @class SocketErrorException
     * @brief Exception class for socket related errors.
     */
    class SocketErrorException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `SocketErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit SocketErrorException(std::string detail = "") noexcept;
    }; // SocketErrorException

    /**
     * @class DnsParseErrorException
     * @brief Exception class for DNS parsing errors.
     */
    class DnsParseErrorException final : public BaseCustomException<Enums::DnsRCodes> {
    public:
        /**
         * @brief Constructor for `DnsParseErrorException`.
         *
         * @param code The DNS RCODE representing the error.
         * @param detail Additional information about the error.
         */
        explicit DnsParseErrorException(Enums::DnsRCodes code, std::string detail = "") noexcept;
    }; // DnsParseErrorException

    /**
     * @class ConnectionErrorException
     * @brief Exception class for connection related errors.
     */
    class ConnectionErrorException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `ConnectionErrorException`.
         *
         * @param detail Additional information about the error.
         */
        explicit ConnectionErrorException(std::string detail = "") noexcept;
    }; // ConnectionErrorException

    /**
     * @class ProtocolErrorException
     * @brief Exception class for protocol errors.
     */
    class ProtocolErrorException final : public BaseCustomException<Enums::ExitCodes> {
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
    class HostnameResolutionErrorException final : public BaseCustomException<Enums::ExitCodes> {
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
    class UserInterruptionException final : public BaseCustomException<Enums::ExitCodes> {
    public:
        /**
         * @brief Constructor for `UserInterruptionException`.
         *
         * @param detail Additional information about the error.
         */
        explicit UserInterruptionException(std::string detail = "") noexcept;
    }; // UserInterruptionException
} // FilteringDnsResolver::Exceptions

#endif // CUSTOM_EXCEPTIONS_HPP

/*** end of file CustomExceptions.hpp ***/
