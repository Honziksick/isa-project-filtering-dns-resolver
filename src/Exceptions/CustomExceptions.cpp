/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         CustomExceptions.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    07.10.2025                                                    *
 *                                                                             *
 * Description:  Implementation file for the `BaseCustomException` class used  *
 *               in the Filtering DNS Resolver.                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file CustomExceptions.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation file for the `CustomExceptions` classes.
 */

#include "Exceptions/CustomExceptions.hpp"
#include "Constants/ExceptionMessages.hpp"
#include "Enums/ExitCodes.hpp"
#include "Enums/DnsRCodes.hpp"
#include <string>   // std::string
#include <utility>  // std::move

using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Enums;
using namespace std;

namespace FilteringDnsResolver::Exceptions
{
    HelpRequestedException::HelpRequestedException() noexcept
        : BaseCustomException{
            ExitCodes::SUCCESS,
            ExceptionMessages::HELP_REQUESTED_MSG,
            std::string{}
        } {}

    InternalErrorException::InternalErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::INTERNAL_ERROR,
            ExceptionMessages::INTERNAL_ERROR_MSG,
            move(detail)
        } {}

    InvalidArgumentException::InvalidArgumentException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::INVALID_ARGUMENT_ERROR,
            ExceptionMessages::INVALID_ARGUMENT_ERROR_MSG,
            move(detail)
        } {}

    UnknownErrorException::UnknownErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::UNKNOWN_ERROR,
            ExceptionMessages::UNKNOWN_ERROR_MSG,
            move(detail)
        } {}

    InvalidFilterFileContentException::InvalidFilterFileContentException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::INVALID_FILTER_FILE_CONTENT_ERROR,
            ExceptionMessages::INVALID_FILTER_FILE_CONTENT_ERROR_MSG,
            move(detail)
        } {}

    SocketErrorException::SocketErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::SOCKET_ERROR,
            ExceptionMessages::SOCKET_ERROR_MSG,
            move(detail)
        } {}

    DnsParseErrorException::DnsParseErrorException(const DnsRCodes code, string detail) noexcept
        : BaseCustomException{
            code,
            string{},
            move(detail)
        } {}

    ConnectionErrorException::ConnectionErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::CONNECTION_ERROR,
            ExceptionMessages::CONNECTION_ERROR_MSG,
            move(detail)
        } {}

    ProtocolErrorException::ProtocolErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::PROTOCOL_ERROR,
            ExceptionMessages::PROTOCOL_ERROR_MSG,
            move(detail)
        } {}

    HostnameResolutionErrorException::HostnameResolutionErrorException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::HOSTNAME_RESOLUTION_ERROR,
            ExceptionMessages::HOSTNAME_RESOLUTION_ERROR_MSG,
            move(detail)
        } {}

    UserInterruptionException::UserInterruptionException(string detail) noexcept
        : BaseCustomException{
            ExitCodes::SUCCESS,
            ExceptionMessages::USER_INTERRUPTION_MSG,
            move(detail)
        } {}
} // FilteringDnsResolver::Exceptions

/*** end of file CustomExceptions.cpp ***/
