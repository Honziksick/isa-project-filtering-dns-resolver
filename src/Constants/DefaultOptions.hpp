/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DefaultOptions.hpp                                            *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    30.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains default values used in the Filtering       *
 *               DNS Resolver application.                                     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DefaultOptions.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file containing default values used accross the project.
 */

#ifndef DEFAULT_OPTIONS_HPP
#define DEFAULT_OPTIONS_HPP

#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Constants
{
    /**
     * @class DefaultOptions
     * @brief Class containing default values for CLI options and other parts of
     *        the program.
     */
    class DefaultOptions {
    public:
        static constexpr uint16_t DEFAULT_LISTENER_PORT = 53;  /**< Default local listening port number.  */
        static constexpr uint16_t DEFAULT_RESOLVER_PORT = 53;  /**< Default DNS resolver port number.     */
    }; // DefaultCliOptions
} // FilteringDnsResolver::Constants

#endif // DEFAULT_OPTIONS_HPP

/*** end of file DefaultOptions.hpp ***/
