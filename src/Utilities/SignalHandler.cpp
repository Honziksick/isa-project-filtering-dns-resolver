/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         SignalHandler.cpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    23.09.2025                                                    *
 *                                                                             *
 * Description:  Implementation of the `SignalHandler` class, which is         *
 *               responsible for handling system signals in a safe and         *
 *               controlled manner.                                            *
 *                                                                             *
 ******************************************************************************/
/**
 * @file SignalHandler.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of the `SignalHandler` class, which provides functionality
 *        for handling system signals (e.g., SIGINT, SIGSEGV) in a thread-safe way.
 */

#include "Utilities/SignalHandler.hpp"
#include "Utilities/Logger.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include <atomic>   // std::atomic
#include <csignal>  // signal

using namespace FilteringDNSResolver::Enums;
using namespace FilteringDNSResolver::Exceptions;

namespace FilteringDNSResolver::Utilities
{
    // Initialization of the static atomic flags
    std::atomic<bool> SignalHandler::mSigintReceived{false};

    void SignalHandler::registerHandlers() {
        logger("Registering signal handlers");

        signal(SIGINT, handleSignal);
        signal(SIGSEGV, handleSignal);
    } // SignalHandler::registerHandlers()

    void SignalHandler::handleSignal(const int signal) {
        logger("Handling signal: %d", signal);

        if(signal == SIGINT) {
            mSigintReceived = true;
        }
        if(signal == SIGSEGV) {
            throw InternalErrorException("Signal SIGSEGV received.");
        }
    } // SignalHandler::handleSignal

    void SignalHandler::checkSignals() {
        if(mSigintReceived) {
            throw UserInterruptionException("Signal SIGINT received.");
        }
    } // SignalHandler::checkSignals
} // FilteringDNSResolver::Utilities

/*** end of file SignalHandler.cpp ***/
