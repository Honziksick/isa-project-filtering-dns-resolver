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
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/Logger.hpp"
#include <exception>  // std::exception

using namespace FilteringDNSResolver::Arguments;
using namespace FilteringDNSResolver::Configurators;
using namespace FilteringDNSResolver::Filter;
using namespace FilteringDNSResolver::Utilities;
using namespace std;

namespace FilteringDNSResolver::Facades
{
    void MainAppFacade::runResolver(const int argc, char *argv[]) {
        try {
            getCommandLineOptions(argc, argv);
            loadAndProcessFilterFileContent(mCommandLineOptions.mFilterFilePath);
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

    void MainAppFacade::loadAndProcessFilterFileContent(const string &filterFilePath) {
        logger("Loading and processing filter file content...");
        mFilterFileContent = FilterFileLoader::loadFilter(filterFilePath);
        logger("Filter file content loaded successfully");
    } // MainAppFacade::getCommandLineOptions()

    void MainAppFacade::getResolverAddress(const string &resolverHostname) {
        logger("Resolving upstream DNS server address...");
        mResolverAddress = ResolverSetup::setupResolver(resolverHostname);
        logger("Upstream DNS server address resolved successfully");
    } // MainAppFacade::getResolverAddress()
} // FilteringDNSResolver::Facades

/*** end of file MainAppFacade.cpp ***/
