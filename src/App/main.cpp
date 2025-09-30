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

using namespace FilteringDNSResolver;
using namespace std;

int main(const int argc, char *argv[]) {
    logger("Starting Filtering DNS Resolver application");
    try {
        // Registers the signal handlers (SIGINT. SIGSEGV)
        Utilities::SignalHandler::registerHandlers();

        // Gives the control to the MainAppFacade
        Facades::MainAppFacade appFacade;
        appFacade.runResolver(argc, argv);
    }
    catch(const exception &e) {
        logger("Exception caught in main(): %s", e.what());
        Utilities::ExceptionHandler::handleError(e, Utilities::ExceptionHandler::TERMINATE);
    }

    logger("Successfully exiting Filtering DNS Resolver application");
    return EXIT_SUCCESS;
} // main()

/*** end of file main.cpp ***/
