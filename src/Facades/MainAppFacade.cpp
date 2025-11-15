/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         MainAppFacade.cpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      28.09.2025                                                    *
 * Last edit:    15.11.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `MainAppFacade` class, which  *
 *               serves as the main application facade for the Filtering DNS   *
 *               Resolver. It implements the facade design pattern to provide  *
 *               a simplified interface to the complex DNS resolver subsystem, *
 *               coordinating initialization, configuration, and execution of  *
 *               all application components including argument parsing, domain *
 *               filtering, network setup, and UDP finite state machine.       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file MainAppFacade.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `MainAppFacade` class for main
 *        application coordination and system initialization management.
 */

#include "Facades/MainAppFacade.hpp"
#include "Arguments/ArgumentParser.hpp"
#include "HostnameResolution/ResolverSetup.hpp"
#include "Filter/FilterFileLoader.hpp"
#include "Filter/DomainFilter.hpp"
#include "Networking/UdpSockets.hpp"
#include "Networking/UdpFsm.hpp"
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/Logger.hpp"
#include <exception>  // std::exception
#include <netdb.h>    // sockaddr_in
#include <string>     // std::string
#include <memory>     // std::make_unique

bool gIsVerboseSet = false;

using namespace FilteringDnsResolver::Arguments;
using namespace FilteringDnsResolver::HostnameResolution;
using namespace FilteringDnsResolver::Filter;
using namespace FilteringDnsResolver::Networking;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::Facades
{
    void MainAppFacade::runResolver(const int argc, char *argv[]) {
        try {
            // First we parse command line options
            getCommandLineOptions(argc, argv);

            // Set the global verbose flag
            gIsVerboseSet = mCommandLineOptions.mVerbose;

            // After we create the domain filter instance
            buildDomainFilter(mCommandLineOptions.mFilterFilePath);

            // Next we resolve the upstream DNS server address
            getResolverAddress(mCommandLineOptions.mResolverHostname);

            // Then we set up UDP sockets for listening and sending DNS queries
            setupUdpSockets(mResolverAddress, mCommandLineOptions.mListenPort);

            // After that we set up the UDP FSM
            setupUdpFsm();

            // Finally we run the UDP FSM
            runUdpFsm();
        }
        catch(const exception &e) {
            ExceptionHandler::handleError(e, ExceptionHandler::TERMINATE);
        }
    } // MainAppFacade::runResolver

    void MainAppFacade::getCommandLineOptions(const int argc, char *argv[]) {
        logger("Parsing command line options...");
        mCommandLineOptions = ArgumentParser::parseArguments(argc, argv);
        logger("Command line options parsed successfully");
    } // MainAppFacade::getCommandLineOptions

    void MainAppFacade::getResolverAddress(const string &resolverHostname) {
        logger("Resolving resolver DNS server address...");
        mResolverAddress = ResolverSetup::setupResolver(resolverHostname);
        logger("Resolver DNS server address resolved successfully");
    } // MainAppFacade::getResolverAddress

    void MainAppFacade::buildDomainFilter(const string &filterFilePath) {
        logger("Building domain filter...");
        vector<string> exactDomains{};
        vector<string> wildcardPatterns{};
        FilterFileLoader::loadFilter(filterFilePath, exactDomains, wildcardPatterns);
        mDomainFilterPtr = make_unique<DomainFilter>(move(exactDomains), move(wildcardPatterns));
        logger("Domain filter built successfully");
    } // MainAppFacade::buildDomainFilter

    void MainAppFacade::setupUdpSockets(const sockaddr_storage &resolverAddress, const uint16_t listenerPort) {
        logger("Setting up UDP sockets...");
        mUdpSocketsPtr = UdpSockets::openUdpSockets(resolverAddress, listenerPort);
        logger("UDP sockets set up successful");
    } // MainAppFacade::setupUdpSockets

    void MainAppFacade::setupUdpFsm() {
        logger("Setting up UDP FSM...");
        mUdpFsmPtr = make_unique<UdpFsm>(move(mUdpSocketsPtr), move(mDomainFilterPtr));
        logger("UDP FSM set up successful");
    } // MainAppFacade::setupUdpFsm

    void MainAppFacade::runUdpFsm() const {
        logger("Running the UDP FSM...");
        mUdpFsmPtr->run();
        logger("UDP FSM terminated successfully");
    } // MainAppFacade::runUdpFsm
} // FilteringDnsResolver::Facades

/*** end of file MainAppFacade.cpp ***/
