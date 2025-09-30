/*******************************************************************************
 *                                                                             *
 * Project:      OMEGA L4 Scanner                                              *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      IPK: Computer Communications and Networks                     *
 *                                                                             *
 * File:         MainAppFacade.cpp                                            *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      20.03.2025                                                    *
 * Last edit:    22.03.2025                                                    *
 *                                                                             *
 * Description:  This file contains the implementation of the MainAppFacade   *
 *               class, which serves as a facade for the OMEGA L4 Scanner      *
 *               application. The facade pattern is used to provide a          *
 *               simplified interface to a complex subsystem.                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file MainAppFacade.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation file for the MainAppFacade class.
 */

#include "Facades/MainAppFacade.hpp"
#include "Arguments/ArgumentParser.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/Logger.hpp"
#include <exception>  // std::exception
#include <cstdlib>    // std::exit

using namespace FilteringDNSResolver::Arguments;
using namespace FilteringDNSResolver::Exceptions;
using namespace FilteringDNSResolver::Utilities;
using namespace std;

namespace FilteringDNSResolver::Facades
{
    MainAppFacade::MainAppFacade() = default;

    void MainAppFacade::runResolver(const int argc, char *argv[]) {
        try {
            getCommandLineOptions(argc, argv);


            // getInterfaceInfo();

            // const ScannerController scannerController(mCommandLineOptions, mInterfaceInfo);
            // scannerController.scanL4Layer();
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
} // OmegaL4Scanner::Facades

/*** end of file MainAppFacade.cpp ***/
