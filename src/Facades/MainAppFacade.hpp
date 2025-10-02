/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         MainAppFacade.hpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      26.09.2025                                                    *
 * Last edit:    30.09.2025                                                    *
 *                                                                             *
 * Description: This file contains the declaration of the `MainAppFacade`      *
 *              class, which serves as a facade for the Filtering DNS Resolver *
 *              application. The facade pattern is used to provide a           *
 *              simplified interface to a complex subsystem.                   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file MainAppFacade.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file for the `MainAppFacade` class.
 */

#ifndef MAIN_APP_FACADE_HPP
#define MAIN_APP_FACADE_HPP

#include "Arguments/CommandLineOptions.hpp"
#include "Filter/DomainFilter.hpp"
#include "Networking/UdpSockets.hpp"
#include <netdb.h>  // sockaddr_in
#include <memory>
#include <string>

namespace FilteringDNSResolver::Facades
{
    /**
     * @class MainAppFacade
     * @brief Facade class for the Filtering DNS Resolver application.
     *
     * @details
     */
    class MainAppFacade final {
    public:
        /**
         * @brief Default constructor for the `MainAppFacade` class.
         */
        explicit MainAppFacade() = default;

        /**
         * @brief Facade class for the Filtering DNS Resolver application.
         *
         * @details
         *
         * @param argc
         * @param argv
         */
        void runResolver(int argc, char *argv[]);

    private:
        Arguments::CommandLineOptions mCommandLineOptions{};   /**< Parsed command line options. */
        std::unique_ptr<Filter::DomainFilter> mDomainFilterPtr{nullptr};          /**< Pointer to the domain filter instance. */
        sockaddr_in mResolverAddress{};                        /**< Resolved upstream DNS server address info. */
        Networking::UdpSockets mUdpSockets{};                     /**< UDP sockets for communication. */

        /**
         * @brief Initializes command line options by parsing the input arguments.
         *
         * @param argc Number of command line arguments.
         * @param argv Array of argument strings.
         */
        void getCommandLineOptions(int argc, char *argv[]);

        void getResolverAddress(const std::string &resolverHostname);

        void buildDomainFilter(const std::string &filterFilePath);

        void setupUdpSockets(uint16_t listenerPort);
    }; // MainAppFacade
} // FilteringDNSResolver::Facades

#endif // MAIN_APP_FACADE_HPP

/*** end of file MainAppFacade.hpp ***/
