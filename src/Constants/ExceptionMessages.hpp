/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ExceptionMessages.hpp                                         *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  This file contains constant exception messages used in the    *
 *               Filtering DNS Resolver project.                               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ExceptionMessages.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file containing exception messages for the Filtering DNS
 *        Resolver project.
 */

#ifndef EXCEPTION_MESSAGES_HPP
#define EXCEPTION_MESSAGES_HPP

namespace FilteringDnsResolver::Constants
{
    /**
     * @class ExceptionMessages
     * @brief Class containing constant exception messages used in the
     *        Filtering DNS Resolver project.
     */
    class ExceptionMessages {
    public:
        /**
         * @brief Message indicating that the user requested help.
         */
        static constexpr auto HELP_REQUESTED_MSG = "User requested help.";

        /**
         * @brief Error message for invalid argument.
         */
        static constexpr auto INVALID_ARGUMENT_ERROR_MSG = "Invalid argument provided.";

        /**
         * @brief Error message for internal error.
         */
        static constexpr auto INTERNAL_ERROR_MSG = "Internal error occurred.";

        /**
         * @brief Error message for unknown error.
         */
        static constexpr auto UNKNOWN_ERROR_MSG = "An unexpected unknown error occurred. Please report this issue to the developers.";

        /**
         * @brief Error message for hostname resolution error.
         */
        static constexpr auto HOSTNAME_RESOLUTION_ERROR_MSG = "Hostname resolution error occurred.";

        /**
         * @brief Error message for internal error.
         */
        static constexpr auto INVALID_FILTER_FILE_CONTENT_ERROR_MSG = "The provided filter file contains invalid content.";

        /**
         * @brief Error message for socket related error.
         */
        static constexpr auto SOCKET_ERROR_MSG = "A problem occurred with the socket.";

        /**
         * @brief Error message for protocol error.
         */
        static constexpr auto PROTOCOL_ERROR_MSG = "Protocol error occurred.";

        /**
         * @brief Error message for user interruption.
         */
        static constexpr auto USER_INTERRUPTION_MSG = "Operation was interrupted by SIGINT signal (i.e., CTRL+C).";
    }; // ExceptionMessages
} // FilteringDnsResolver::Constants

#endif // EXCEPTION_MESSAGES_HPP

/*** end of file ExceptionMessages.hpp ***/
