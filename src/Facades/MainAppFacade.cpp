/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         MainAppFacade.cpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      20.03.2025                                                    *
 * Last edit:    22.03.2025                                                    *
 *                                                                             *
 * Description:  This file contains the implementation of the `MainAppFacade`  *
 *              class, which serves as a facade for the Filtering DNS Resolver *
 *              application. The facade pattern is used to provide a           *
 *              simplified interface to a complex subsystem.                   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file MainAppFacade.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation file for the `MainAppFacade` class.
 */

#include "Facades/MainAppFacade.hpp"
#include "Arguments/ArgumentParser.hpp"
#include "Configurators/ResolverSetup.hpp"
#include "Filter/FilterFileLoader.hpp"
#include "Filter/DomainFilter.hpp"
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/Logger.hpp"
#include <exception>  // std::exception
#include <memory>     // std::make_unique
#include <string>
#include <vector>

using namespace FilteringDNSResolver::Arguments;
using namespace FilteringDNSResolver::Configurators;
using namespace FilteringDNSResolver::Filter;
using namespace FilteringDNSResolver::Utilities;
using namespace std;

namespace FilteringDNSResolver::Facades
{
    void MainAppFacade::runResolver(const int argc, char *argv[]) {
        try {
            // First we parse command line options
            getCommandLineOptions(argc, argv);

            // After we create the domain filter instance
            buildDomainFilter(mCommandLineOptions.mFilterFilePath);

            // Next we resolve the upstream DNS server address
            getResolverAddress(mCommandLineOptions.mResolverHostname);

            // TODO
        }
        catch(const exception &e) {
            ExceptionHandler::handleError(e, ExceptionHandler::TERMINATE);
        }
    } // MainAppFacade::runResolver()

    void MainAppFacade::getCommandLineOptions(const int argc, char *argv[]) {
        logger("Parsing command line options...");
        mCommandLineOptions = ArgumentParser::parseArguments(argc, argv);
        logger("Command line options parsed successfully");
    } // MainAppFacade::getCommandLineOptions()

    void MainAppFacade::getResolverAddress(const string &resolverHostname) {
        logger("Resolving resolver DNS server address...");
        mResolverAddress = ResolverSetup::setupResolver(resolverHostname);
        logger("Resolver DNS server address resolved successfully");
    } // MainAppFacade::getResolverAddress()

    void MainAppFacade::buildDomainFilter(const string &filterFilePath) {
        logger("Building domain filter...");
        mDomainFilterPtr = make_unique<DomainFilter>(FilterFileLoader::loadFilter(filterFilePath));
        logger("Domain filter built successfully");
    } // MainAppFacade::buildDomainFilter()
} // FilteringDNSResolver::Facades

/*** end of file MainAppFacade.cpp ***/
