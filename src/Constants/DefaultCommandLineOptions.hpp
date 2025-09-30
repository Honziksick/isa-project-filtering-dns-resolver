/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DefaultCommandLineOptions.hpp                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    23.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains default values for command line options    *
 *               used in the Filtering DNS Resolver application.               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DefaultCommandLineOptions.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file containing default values for command line options.
 */

#ifndef DEFAULT_COMMAND_LINE_OPTIONS_HPP
#define DEFAULT_COMMAND_LINE_OPTIONS_HPP

#include <cstdint>  // uint8_t, uint16_t

namespace FilteringDNSResolver::Constants
{
    /**
     * @class DefaultCliOptions
     * @brief Class containing default values for command line options.
     */
    class DefaultCliOptions {
    public:
        static constexpr uint16_t DEFAULT_DNS_PORT = 53;  /**< Default server port number. */
    }; // DefaultCliOptions
} // FilteringDNSResolver::Constants

#endif // DEFAULT_COMMAND_LINE_OPTIONS_HPP

/*** end of file DefaultCommandLineOptions.hpp ***/
