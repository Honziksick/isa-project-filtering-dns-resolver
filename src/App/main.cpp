/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         main.cpp                                                      *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains the main function that serves as the       *
 *               entry point for the ISA Filtering DNS Resolver application.   *
 *               It initializes the application.                               *
 *                                                                             *
 ******************************************************************************/
/**
 * @file main.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Main entry point for the ISA Filtering DNS Resolver application.
 */

#include "Facades/MainAppFacade.hpp"
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/SignalHandler.hpp"
#include "Utilities/Logger.hpp"
#include <exception>  // std::exception

using namespace FilteringDnsResolver;
using namespace std;

int main(const int argc, char *argv[]) {
    logger("Starting Filtering DNS Resolver application with %d arguments", argc);
    verbose("Initializing DNS resolver...");
    try {
        // Registers the signal handlers (SIGINT. SIGSEGV)
        logger("Registering signal handlers for graceful shutdown and crash handling");
        Utilities::SignalHandler::registerHandlers();
        logger("Signal handlers registered successfully");

        // Gives the control to the MainAppFacade
        logger("Creating MainAppFacade instance for application control");
        Facades::MainAppFacade appFacade;
        logger("Initializing resolver with command line arguments: argc=%d", argc);
        appFacade.runResolver(argc, argv);
        logger("MainAppFacade.runResolver() completed successfully");
    }
    catch(const exception &e) {
        logger("Exception caught in main(): %s", e.what());
        verbose("Error: DNS resolver failed to start - %s", e.what());
        Utilities::ExceptionHandler::handleError(e, Utilities::ExceptionHandler::TERMINATE);  // sanity handle
    }

    logger("Successfully exiting Filtering DNS Resolver application with exit code %d", EXIT_SUCCESS);
    verbose("DNS resolver shutdown complete");
    return EXIT_SUCCESS;
} // main()

/*** end of file main.cpp ***/
