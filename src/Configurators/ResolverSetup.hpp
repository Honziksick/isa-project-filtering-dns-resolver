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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ResolverSetup.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef RESOLVER_SETUP_HPP
#define RESOLVER_SETUP_HPP

#include "Configurators/HostnameResolver.hpp"
#include <netdb.h>  // sockaddr_in
#include <string>

namespace FilteringDnsResolver::Configurators
{
    class ResolverSetup final : HostnameResolver {
    public:
        static sockaddr_in setupResolver(const std::string& resolverHostname);
    }; // ResolverSetup
} // FilteringDnsResolver::Configurators

#endif // RESOLVER_SETUP_HPP

/*** end of file ResolverSetup.hpp ***/
