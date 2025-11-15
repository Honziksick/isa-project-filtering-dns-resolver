/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ResolverSetup.hpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      29.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `ResolverSetup` class, which        *
 *               extends `HostnameResolver` to provide specialized             *
 *               functionality for setting up upstream DNS resolver            *
 *               configurations.                                               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ResolverSetup.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `ResolverSetup` class for configuring upstream
 *        DNS resolver connections.
 */

#ifndef RESOLVER_SETUP_HPP
#define RESOLVER_SETUP_HPP

#include "HostnameResolution/HostnameResolver.hpp"
#include <sys/socket.h>  // sockaddr_storage
#include <netdb.h>       // sockaddr_storage
#include <string>        // std::string

namespace FilteringDnsResolver::HostnameResolution
{
    /**
     * @class ResolverSetup
     * @brief Provides methods for setting up upstream DNS resolver configurations.
     *
     * @details This class inherits from `HostnameResolver` and specializes in
     *          creating `sockaddr_in` structures for upstream DNS resolver
     *          connections. It handles the resolution and configuration of
     *          DNS server addresses that will be used for forwarding queries.
     */
    class ResolverSetup final : public HostnameResolver {
    public:
        /**
         * @brief Sets up a DNS resolver configuration from a hostname.
         *
         * @details This method resolves the provided hostname to create a
         *          `sockaddr_in` structure configured for DNS communication
         *          on the standard DNS port (53). The resulting structure
         *          can be used to establish connections with upstream DNS
         *          resolvers for query forwarding.
         *
         * @param resolverHostname The hostname or IP address of the DNS resolver
         *                         to configure.
         *
         * @return A `sockaddr_storage` structure containing the resolved
         *         address and port configuration for the DNS resolver.
         *
         * @throws HostnameResolutionErrorException if the hostname cannot be
         *         resolved or if the resolved address is invalid.
         */
        static sockaddr_storage setupResolver(const std::string& resolverHostname);
    }; // ResolverSetup
} // FilteringDnsResolver::HostnameResolution

#endif // RESOLVER_SETUP_HPP

/*** end of file ResolverSetup.hpp ***/
