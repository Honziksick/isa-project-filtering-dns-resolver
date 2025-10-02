/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         HostnameResolver.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    30.09.2025                                                    *
 *                                                                             *
 * Description:  This file implements of the `HostnameResolver` class,         *
 *               which provides functions for resolving the provided server    *
 *               hostname.                                                     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file HostnameResolver.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of `HostnameResolver` class providing the methods for
 *        resolving the provided server hostname.
 */

#include "Configurators/HostnameResolver.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Constants/DefaultOptions.hpp"
#include "Utilities/Logger.hpp"
#include <sys/socket.h>  // AF_INET, SOCK_DGRAM
#include <netdb.h>       // addrinfo, getaddrinfo(), gai_strerror(), freeaddrinfo()
#include <cstring>       // std::memset
#include <string>        // std::string

using namespace FilteringDNSResolver::Exceptions;
using namespace FilteringDNSResolver::Constants;
using namespace std;

namespace FilteringDNSResolver::Configurators
{
    addrinfo *HostnameResolver::resolveHostname(const string &hostname) {
        logger("Starting hostname resolution for hostname: '%s'", hostname.c_str());

        // Prepare the 'hints' structure for address resolution
        addrinfo hints{};
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_INET;        // IPv4 addresses
        hints.ai_socktype = SOCK_DGRAM;   // UDP
        hints.ai_flags = AI_NUMERICSERV;  // port is a number

        logger("Hints prepared: 'ai_family = %d', 'ai_socktype = %d', 'ai_flags = %d'",
               hints.ai_family, hints.ai_socktype, hints.ai_flags);

        // Perform the address resolution
        addrinfo *pResult{nullptr};
        const int getaddrinfoError = getaddrinfo(hostname.c_str(),
                                                 to_string(DefaultOptions::DEFAULT_RESOLVER_PORT).c_str(),
                                                 &hints,
                                                 &pResult);

        // Handle potential errors from getaddrinfo()
        if(getaddrinfoError != 0) {
            logger("getaddrinfo() error: %s", gai_strerror(getaddrinfoError));
            switch(getaddrinfoError) {
                case EAI_NONAME:
                    throw InvalidArgumentException(
                            "The specified hostname does not exist. Please check the provided hostname and try again."
                            );
                default:
                    throw HostnameResolutionErrorException(
                            "When trying to resolve host, an error occured: " + string(gai_strerror(getaddrinfoError))
                            );
            }
        }

        logger("Hostname resolution successful. Returning addrinfo structure.");
        return pResult;
    } // HostnameResolver::resolveHostname()
} // FilteringDNSResolver::Utilities

/*** end of file HostnameResolver.cpp ***/
