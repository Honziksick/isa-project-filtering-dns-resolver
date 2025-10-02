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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ResolverSetup.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "Configurators/ResolverSetup.hpp"
#include "Arguments/CommandLineOptions.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Constants/DefaultOptions.hpp"
#include "Utilities/Logger.hpp"
#include <netdb.h>  // addrinfo, sockaddr_in, freeaddrinfo()
#include <cstring>  // memcpy

using namespace FilteringDNSResolver::Arguments;
using namespace FilteringDNSResolver::Exceptions;
using namespace FilteringDNSResolver::Constants;
using namespace std;

namespace FilteringDNSResolver::Configurators
{
    sockaddr_in ResolverSetup::setupResolver(const string &resolverHostname) {
        logger("Starting resolver setup");

        // First we need to resolve the upstream DNS server address
        addrinfo *resolvedAddressInfo = resolveHostname(resolverHostname);
        logger("Host resolution completed");

        // Then we need to copy the resolved address into a sockaddr_in structure
        sockaddr_in resolverAddress{};
        if(resolvedAddressInfo && resolvedAddressInfo->ai_addr &&
            resolvedAddressInfo->ai_addrlen >= sizeof(sockaddr_in)) {
            // Copy the resolved address into the sockaddr_in structure and set its parameters
            memcpy(&resolverAddress, resolvedAddressInfo->ai_addr, sizeof(sockaddr_in));
            resolverAddress.sin_family = AF_INET;
            resolverAddress.sin_port = htons(DefaultOptions::DEFAULT_RESOLVER_PORT);
        }
        else {
            freeaddrinfo(resolvedAddressInfo);
            throw HostnameResolutionErrorException(
                "Resolved address for '" + resolverHostname + "' is incomplete or invalid"
            );
        }

        freeaddrinfo(resolvedAddressInfo);  // Free the addrinfo structure after use
        logger("Resolver setup finished");

        return resolverAddress;
    } // ResolverSetup::setupResolver
} // FilteringDNSResolver::Configurators

/*** end of file ResolverSetup.cpp ***/
