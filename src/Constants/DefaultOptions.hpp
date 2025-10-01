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
 * Description:  This file contains default values for command line options    *
 *               used in the Filtering DNS Resolver application.               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DefaultOptions.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file containing default values for command line options.
 */

#ifndef DEFAULT_OPTIONS_HPP
#define DEFAULT_OPTIONS_HPP

#include <cstdint>  // uint8_t, uint16_t

namespace FilteringDNSResolver::Constants
{
    /**
     * @class DefaultOptions
     * @brief Class containing default values for CLI options and other parts of
     *        the program.
     */
    class DefaultOptions {
    public:
        static constexpr uint16_t DEFAULT_LISTEN_PORT = 53;     /**< Default local listening port number.     */
        static constexpr uint8_t  DEFAULT_UPSTREAM_PORT = 53;   /**< Default upstream DNS server port number. */
    }; // DefaultCliOptions
} // FilteringDNSResolver::Constants

#endif // DEFAULT_OPTIONS_HPP

/*** end of file DefaultOptions.hpp ***/
