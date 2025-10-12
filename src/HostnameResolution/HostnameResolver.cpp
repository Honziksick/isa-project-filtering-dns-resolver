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

#include "HostnameResolution/HostnameResolver.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Constants/DefaultOptions.hpp"
#include "Utilities/Logger.hpp"
#include <sys/socket.h>  // AF_INET, SOCK_DGRAM
#include <netdb.h>       // addrinfo, getaddrinfo(), gai_strerror(), freeaddrinfo()
#include <cstring>       // std::memset
#include <string>        // std::string

using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Constants;
using namespace std;

namespace FilteringDnsResolver::HostnameResolution
{
    addrinfo *HostnameResolver::resolveHostname(const string &hostname) {
        logger("HostnameResolver::resolveHostname() called with hostname: '%s' "
               "(length=%zu)", hostname.c_str(), hostname.length());
        verbose("Resolving DNS server hostname: %s", hostname.c_str());

        // Prepare the 'hints' structure for address resolution
        logger("Preparing addrinfo hints structure");
        addrinfo hints{};
        memset(&hints, 0, sizeof(hints));
        logger("addrinfo hints structure zeroed (size=%zu bytes)", sizeof(hints));

        hints.ai_family = AF_INET;        // IPv4 addresses
        hints.ai_socktype = SOCK_DGRAM;   // UDP
        hints.ai_flags = AI_NUMERICSERV;  // port is a number

        logger("addrinfo hints configured:");
        logger("  ai_family = %d (AF_INET for IPv4)", hints.ai_family);
        logger("  ai_socktype = %d (SOCK_DGRAM for UDP)", hints.ai_socktype);
        logger("  ai_flags = %d (AI_NUMERICSERV for numeric port)", hints.ai_flags);

        // Perform the address resolution
        logger("Calling getaddrinfo() for hostname resolution");
        logger("Using port: %s (DEFAULT_RESOLVER_PORT=%d)",
               to_string(DefaultOptions::DEFAULT_RESOLVER_PORT).c_str(),
               DefaultOptions::DEFAULT_RESOLVER_PORT);

        logger("Calling getaddrinfo('%s', '%s', hints, &pResult)",
               hostname.c_str(), to_string(DefaultOptions::DEFAULT_RESOLVER_PORT).c_str());
        addrinfo *pResult{nullptr};
        const int getaddrinfoError = getaddrinfo(hostname.c_str(),
                                                 to_string(DefaultOptions::DEFAULT_RESOLVER_PORT).c_str(),
                                                 &hints,
                                                 &pResult);
        logger("getaddrinfo() returned with code: %d", getaddrinfoError);

        // Handle potential errors from getaddrinfo()
        if(getaddrinfoError != 0) {
            logger("getaddrinfo() error: %s", gai_strerror(getaddrinfoError));

            switch(getaddrinfoError) {
                case EAI_NONAME:
                    logger("Error type: EAI_NONAME - hostname does not exist");
                    verbose("Failed to resolve hostname '%s': hostname not found", hostname.c_str());

                    throw InvalidArgumentException(
                            "The specified hostname does not exist. Please check the provided hostname and try again."
                            );
                default:
                    logger("Error type: internal hostname resolution error: %s", gai_strerror(getaddrinfoError));
                    verbose("Failed to resolve hostname '%s': internal hostname resolution error", hostname.c_str());

                    throw HostnameResolutionErrorException(
                            "When trying to resolve host, an error occured: " + string(gai_strerror(getaddrinfoError))
                            );
            } // switch
        } // if

        logger("getaddrinfo() succeeded, analyzing results");
        if(pResult == nullptr) {
            logger("ERROR: pResult is nullptr despite successful getaddrinfo()");
            verbose("Internal error: no address information returned");
            throw HostnameResolutionErrorException(
                    "No address information returned for hostname: " + hostname
                    );
        }

        logger("addrinfo structure returned: Analyzing first result:");
        logger("  ai_family = %d", pResult->ai_family);
        logger("  ai_socktype = %d", pResult->ai_socktype);
        logger("  ai_protocol = %d", pResult->ai_protocol);
        logger("  ai_addrlen = %u", pResult->ai_addrlen);

        logger("Hostname resolution completed successfully for: '%s'", hostname.c_str());
        logger("Returning addrinfo structure");
        return pResult;
    } // HostnameResolver::resolveHostname
} // FilteringDnsResolver::HostnameResolution

/*** end of file HostnameResolver.cpp ***/
