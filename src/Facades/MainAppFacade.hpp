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
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `MainAppFacade` class, which        *
 *               serves as the main application facade for the Filtering DNS   *
 *               Resolver. It implements the facade design pattern to provide  *
 *               a simplified interface to the complex DNS resolver subsystem, *
 *               coordinating initialization, configuration, and execution of  *
 *               all application components including argument parsing, domain *
 *               filtering, network setup, and UDP finite state machine.       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file MainAppFacade.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `MainAppFacade` class for main application
 *        coordination and system initialization management.
 */

#ifndef MAIN_APP_FACADE_HPP
#define MAIN_APP_FACADE_HPP

#include "Arguments/CommandLineOptions.hpp"
#include "Filter/DomainFilter.hpp"
#include "Networking/UdpSockets.hpp"
#include "Networking/UdpFsm.hpp"
#include <netdb.h>  // sockaddr_in
#include <memory>   // std::unique_ptr
#include <string>   // std::string

namespace FilteringDnsResolver::Facades
{
    /**
     * @class MainAppFacade
     * @brief Main application facade for the Filtering DNS Resolver.
     *
     * @details Implements the facade design pattern to provide a unified interface
     *          for initializing and running the DNS resolver application. Coordinates
     *          argument parsing, domain filtering setup, network configuration,
     *          and UDP finite state machine execution in a structured workflow.
     */
    class MainAppFacade final {
    public:
        /**
         * @brief Default constructor creating an uninitialized facade.
         *
         * @details Creates the facade with default-initialized components.
         *          Application setup occurs during `runResolver()` execution.
         */
        explicit MainAppFacade() = default;

        /**
         * @brief Main entry point for running the DNS resolver application.
         *
         * @details Orchestrates complete application lifecycle including argument
         *          parsing, component initialization, network setup, and main
         *          processing loop execution. Handles all configuration and
         *          startup sequence in proper order.
         *
         * @param argc Number of command line arguments from main().
         * @param argv Array of command line argument strings from main().
         */
        void runResolver(int argc, char *argv[]);

    private:
        Arguments::CommandLineOptions mCommandLineOptions{};              /**< Parsed command line options and configuration.      */
        std::unique_ptr<Filter::DomainFilter> mDomainFilterPtr{nullptr};  /**< Domain filtering engine for query processing.       */
        sockaddr_in mResolverAddress{};                                   /**< Resolved upstream DNS server network address.       */
        std::unique_ptr<Networking::UdpSockets> mUdpSocketsPtr{nullptr};  /**< UDP socket management for network communication.    */
        std::unique_ptr<Networking::UdpFsm> mUdpFsmPtr{nullptr};          /**< UDP finite state machine for DNS protocol handling. */

        /**
         * @brief Parses and validates command line arguments.
         *
         * @details Processes program arguments to extract configuration options
         *          including server address, port, and filter file settings.
         *          Validates argument format and required parameters.
         *
         * @param argc Number of command line arguments.
         * @param argv Array of argument strings.
         */
        void getCommandLineOptions(int argc, char *argv[]);

        /**
         * @brief Resolves upstream DNS server hostname to network address.
         *
         * @details Performs hostname resolution to obtain the network address
         *          of the upstream DNS server for query forwarding.
         *
         * @param resolverHostname Hostname or IP address of upstream DNS server.
         */
        void getResolverAddress(const std::string &resolverHostname);

        /**
         * @brief Initializes domain filtering system from configuration file.
         *
         * @details Loads and parses the domain filter configuration file to
         *          build the filtering engine for processing DNS queries.
         *
         * @param filterFilePath Path to the domain filter configuration file.
         */
        void buildDomainFilter(const std::string &filterFilePath);

        /**
         * @brief Sets up UDP sockets for DNS communication.
         *
         * @details Initializes client and server UDP sockets on the specified
         *          port for receiving DNS queries and forwarding responses.
         *
         * @param resolverAddress Address of upstream DNS resolver (includes IP and port).
         * @param listenerPort Port number for DNS query listener socket.
         */
        void setupUdpSockets(sockaddr_in resolverAddress, uint16_t listenerPort);

        /**
         * @brief Initializes the UDP finite state machine.
         *
         * @details Creates and configures the UDP FSM with all necessary
         *          components for DNS protocol handling and query processing.
         */
        void setupUdpFsm();

        /**
         * @brief Starts the main DNS resolver processing loop.
         *
         * @details Executes the UDP finite state machine to begin processing
         *          incoming DNS queries and managing resolver operations.
         */
        void runUdpFsm() const;
    }; // MainAppFacade
} // FilteringDnsResolver::Facades

#endif // MAIN_APP_FACADE_HPP

/*** end of file MainAppFacade.hpp ***/
