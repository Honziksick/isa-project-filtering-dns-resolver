/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ResolverSetup.cpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      29.09.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  This file implements the `ResolverSetup` class, which         *
 *               extends `HostnameResolver` to provide specialized             *
 *               functionality for setting up upstream DNS resolver            *
 *               configurations.                                               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ResolverSetup.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief File implementing the `ResolverSetup` class for configuring upstream
 *        DNS resolver connections.
 */

#include "HostnameResolution/ResolverSetup.hpp"
#include "Arguments/CommandLineOptions.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Constants/DefaultOptions.hpp"
#include "Utilities/Logger.hpp"
#include <sys/socket.h>  // AF_INET, SOCK_DGRAM
#include <arpa/inet.h>   // inet_ntoa(), htons()
#include <netdb.h>       // addrinfo, sockaddr_in, freeaddrinfo()
#include <cstring>       // memcpy

using namespace FilteringDnsResolver::Arguments;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Constants;
using namespace std;

namespace FilteringDnsResolver::HostnameResolution
{
    sockaddr_in ResolverSetup::setupResolver(const string &resolverHostname) {
        logger("ResolverSetup::setupResolver() called with hostname: '%s' (length=%zu)",
               resolverHostname.c_str(), resolverHostname.length());
        verbose("Setting up upstream DNS resolver: %s", resolverHostname.c_str());

        // First we need to resolve the upstream DNS server address
        logger("Step 1: Resolving upstream DNS server hostname");
        addrinfo *resolvedAddressInfo = resolveHostname(resolverHostname);
        logger("Host resolution completed");

        // Then we need to copy the resolved address into a sockaddr_in structure
        logger("Step 2: Creating sockaddr_in structure and validating resolved address");
        sockaddr_in resolverAddress{};

        if(resolvedAddressInfo && resolvedAddressInfo->ai_addr &&
            resolvedAddressInfo->ai_addrlen >= sizeof(sockaddr_in)) {
            logger("Address validation passed - ai_addrlen=%u, required=%zu",
                   resolvedAddressInfo->ai_addrlen, sizeof(sockaddr_in));

            // Copy the resolved address into the sockaddr_in structure and set its parameters
            logger("Copying address data to sockaddr_in structure");
            memcpy(&resolverAddress, resolvedAddressInfo->ai_addr, sizeof(sockaddr_in));

            resolverAddress.sin_family = AF_INET;
            resolverAddress.sin_port = htons(DefaultOptions::DEFAULT_RESOLVER_PORT);

            // Log the resolved IP for debugging
            const char *ipString = inet_ntoa(resolverAddress.sin_addr);
            logger("Resolver configured: %s:%d", ipString, DefaultOptions::DEFAULT_RESOLVER_PORT);
            verbose("Successfully configured upstream DNS resolver: %s:%d",
                    ipString, DefaultOptions::DEFAULT_RESOLVER_PORT);
        }
        else {
            logger("ERROR: Address validation failed - resolvedAddressInfo=%p, ai_addr=%p, ai_addrlen=%u",
                   static_cast<void*>(resolvedAddressInfo),
                   resolvedAddressInfo ? static_cast<void*>(resolvedAddressInfo->ai_addr) : nullptr,
                   resolvedAddressInfo ? resolvedAddressInfo->ai_addrlen : 0);
            verbose("Failed to configure upstream DNS resolver '%s' - invalid address data", resolverHostname.c_str());

            freeaddrinfo(resolvedAddressInfo);
            throw HostnameResolutionErrorException(
                    "Resolved address for '" + resolverHostname + "' is incomplete or invalid"
                    );
        }

        logger("Step 3: Freeing addrinfo structure");
        freeaddrinfo(resolvedAddressInfo);  // free the addrinfo structure after use
        logger("Resolver setup finished successfully");

        return resolverAddress;
    } // ResolverSetup::setupResolver
} // FilteringDnsResolver::HostnameResolution

/*** end of file ResolverSetup.cpp ***/
