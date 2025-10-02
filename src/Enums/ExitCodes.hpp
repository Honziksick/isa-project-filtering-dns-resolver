/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ExitCodes.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  Declaration of the `ExitCodes` enum class, which is used to   *
 *               represent error and other exit codes in the Filtering DNS     *
 *               Resolver project.                                             *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ExitCodes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file for the `ExitCodes` enum class.
 */

#ifndef EXIT_CODES_HPP
#define EXIT_CODES_HPP

namespace FilteringDNSResolver::Enums
{
    /**
     * @enum ExitCodes
     * @brief Enum class representing custom exit codes in the Filtering DNS
     *        Resolver project.
     *
     * @details This enum class defines various error codes that can be used
     *          to represent different error conditions in the Filtering DNS
     *          Resolver project.
     */
    enum class ExitCodes {
        SUCCESS                           = 0,     /**< Success exit code.                                                */
        INTERNAL_ERROR                    = 1,     /**< Internal error code (EPERM).                                      */
        INVALID_ARGUMENT_ERROR            = 22,    /**< Command line usage error (EINVAL).                                */
        INVALID_FILTER_FILE_CONTENT_ERROR = 23,    /**< Invalid filter file content error code.                           */
        UNKNOWN_ERROR                     = 42,    /**< Unknown error (The Answer to Life, the Universe, and Everything). */
        PROTOCOL_ERROR                    = 71,    /**< Protocol error (EPROTO).                                          */
        SOCKET_ERROR                      = 107,   /**< Socket error code (ENOTCONN).                                     */
        HOSTNAME_RESOLUTION_ERROR         = 113,   /**< Hostname resolution error code (EHOSTUNREACH).                    */
        USER_INTERRUPTION_ERROR           = 130    /**< Process interrupted by user error code (128 + SIGINT).            */
    }; // ExitCodes

} // FilteringDNSResolver::Enums

#endif // EXIT_CODES_HPP

/*** end of file ExitCodes.hpp ***/
