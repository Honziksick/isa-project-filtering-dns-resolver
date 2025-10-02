/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ExceptionHandler.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description:  Declaration of the `ExceptionHandler` class, which is         *
 *               responsible for printing error messages and terminating the   *
 *               application with the appropriate exit code when an error      *
 *               exception is caught.                                          *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ExceptionHandler.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file decalring the `ExceptionHandler` class, which is mainly
 *        responsible for handling error exceptions.
 */

#ifndef EXCEPTION_HANDLER_HPP
#define EXCEPTION_HANDLER_HPP

#include "Exceptions/BaseCustomException.hpp"
#include <exception>  // std::exception

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class ExceptionHandler
     * @brief Class responsible for handling and printing error messages.
     */
    class ExceptionHandler {
    public:
        /**
         * @brief Flag to requesting program termination after handling the error.
         */
        static constexpr bool TERMINATE = true;

        /**
         * @brief Handles the given exception by printing the error message
         *        and terminating the program with error code.
         * @details This method attempts to cast the given exception to a
         *          `BaseCustomException`. If the cast is successful, it prints
         *          the error message and terminates the program with the error
         *          code associated with the exception. If the cast fails, it
         *          prints a generic error message and terminates the program
         *          with a default error code.
         *
         * @param exception The exception to handle.
         * @param terminate Whether to terminate the program after handling the error.
         */
        static void handleError(const std::exception &exception, bool terminate = false);

    private:
        /**
         * @brief Prints the given error message.
         * @param exception The exception containing the error message to print.
         */
        static void printError(const Exceptions::BaseCustomException &exception);

        /**
         * @brief Terminates the program with the given error code.
         * @param errorCode The error code to terminate the program with.
         */
        static void terminateProgram(int errorCode);

        /**
         * @brief Retrieves the BaseCustomException from an exception.
         *
         * @details This method attempts to cast a standard exception to an
         *          BaseCustomException. If the cast is successful, it returns
         *          a pointer to the BaseCustomException. Otherwise, it returns
         *          nullptr.
         *
         * @param exception The standard exception to cast.
         * @return Pointer to the BaseCustomException if cast is successful,
         *         nullptr otherwise.
         */
        static const Exceptions::BaseCustomException *getCustomException(const std::exception &exception);
    }; // ExceptionHandler
} // FilteringDnsResolver::Utilities

#endif // EXCEPTION_HANDLER_HPP

/*** end of file ExceptionHandler.hpp ***/
