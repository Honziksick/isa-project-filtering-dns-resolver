/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ExceptionHandler.cpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Implementation of the `ExceptionHandler` class, which is      *
 *               responsible for printing error messages and terminating the   *
 *               application with the appropriate exit code when an error      *
 *               exception is caught.                                          *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ExceptionHandler.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of the `ExceptionHandler` class, which is mainly
 *        responsible for handling error exceptions.
 */

#include "Exceptions/CustomExceptions.hpp"
#include "Enums/ExitCodes.hpp"
#include "Utilities/ExceptionHandler.hpp"
#include "Utilities/CastUtils.hpp"
#include "Utilities/Logger.hpp"
#include "Constants/ColorEscapeSequences.hpp"
#include <exception>  // std::exception
#include <iostream>   // std::cerr
#include <string>     // std::string

using namespace FilteringDNSResolver::Enums;
using namespace FilteringDNSResolver::Exceptions;
using namespace FilteringDNSResolver::Constants;
using namespace std;

namespace FilteringDNSResolver::Utilities
{
    void ExceptionHandler::handleError(const exception &exception, const bool terminate) {
        logger("Handling error: %s", exception.what());

        // Attempt to cast the original exception to BaseCustomException
        const BaseCustomException *pCustomException = getCustomException(exception);

        // Create an UnknownErrorException (only used if 'dynamic_cast' above failed)
        const UknownErrorException unknownException{exception.what()};

        // If 'dynamic_cast' above failed, use the address of UnknownException
        if(!pCustomException) {
            logger("Unknown exception type, using UknownErrorException");
            pCustomException = &unknownException;
        }

        // Now we are 100% sure that the pCustomException points to BaseCustomException
        if(pCustomException->code() != CastUtils::castEnumToInt(ExitCodes::SUCCESS)) {
            printError(*pCustomException);
        }

        // Terminate the program if requested
        if(terminate) {
            terminateProgram(pCustomException->code());
        }
    } // ExceptionHandler::handleError()

    void ExceptionHandler::printError(const BaseCustomException &exception) {
        cerr << Color::RED << "Error " << exception.code() << ": " << exception.what() << Color::RESET << endl;
        if(!exception.detail().empty()) {
            cerr << Color::YELLOW << "Detail: " << exception.detail() << Color::RESET << endl;
        }
    } // ExceptionHandler::printError()

    void ExceptionHandler::terminateProgram(const int errorCode) {
        logger("Terminating program with error code: %d", errorCode);
        exit(errorCode);
    } // ExceptionHandler::terminateProgram()

    const BaseCustomException *ExceptionHandler::getCustomException(const exception &exception) {
        logger("Getting BaseCustomException from exception: %s", exception.what());
        return dynamic_cast<const BaseCustomException*>(&exception);
    } // ExceptionHandler::getCustomException()
} // FilteringDNSResolver::Exceptions

/*** end of file ExceptionHandler.cpp ***/
