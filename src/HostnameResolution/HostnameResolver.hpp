/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         HostnameResolver.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    30.09.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `HostnameResolver` class, which     *
 *               provides functions for resolving the provided server          *
 *               hostname.                                                     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file HostnameResolver.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `HostnameResolver` class for resolving the
 *        provided server hostname.
 */

#ifndef HOSTNAME_RESOLVER_HPP
#define HOSTNAME_RESOLVER_HPP

#include <string>   // std::string
#include <netdb.h>  // addrinfo

namespace FilteringDnsResolver::HostnameResolution
{
    /**
     * @class HostnameResolver
     * @brief Provides methods for resolving hostnames to addresses.
     */
    class HostnameResolver {
    protected:
        /**
         * @brief Resolves a hostname or IP address to an `addrinfo` structure.
         *
         * @details This method takes a hostname or IP address and a server
         *          port, and resolves them into an `addrinfo` structure that
         *          can be used for creating a socket connection.
         *
         * @param hostname The hostname or IP address to resolve.
         *
         * @return A pointer to an `addrinfo` structure containing the resolved
         *         address information.
         *
         * @warning The caller is responsible for freeing returned structure
         *          using `freeaddrinfo()`.
         *
         * @throws HostnameResolutionErrorException if the hostname cannot be
         *         resolved.
         * @throws InvalidArgumentErrorException if the provided hostname is empty,
         *         or if address resolution fails because of invalid hostname.
         */
        static addrinfo *resolveHostname(const std::string &hostname);
    }; // HostnameResolver
} // FilteringDnsResolver::HostnameResolution

#endif // HOSTNAME_RESOLVER_HPP

/*** end of file HostnameResolver.hpp ***/
