/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         SignalHandler.hpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    23.09.2025                                                    *
 *                                                                             *
 * Description:  Declaration of the `SignalHandler` class, which is            *
 *               responsible for handling system signals in a safe and         *
 *               controlled manner.                                            *
 *                                                                             *
 ******************************************************************************/
/**
 * @file SignalHandler.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file defining the `SignalHandler` class, which provides functionality
 *        for handling system signals (e.g., SIGINT, SIGSEGV) in a thread-safe way.
 */

#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <atomic>  // std::atomic

namespace FilteringDNSResolver::Utilities
{
    /**
     * @class SignalHandler
     * @brief Provides functionality for handling system signals in a safe and
     *        controlled manner.
     */
    class SignalHandler final {
    public:
        /**
         * @brief Registers the necessary signal handlers for the application.
         * @details This function sets up handlers for signals such as SIGINT and
         *          SIGSEGV. These handlers will set atomic flags when the
         *          corresponding signal is received.
         */
        static void registerHandlers();

        /**
         * @brief Checks for received signals and throws appropriate exceptions.
         * @details This function should be called periodically in the main
         *          program loop. It checks the atomic flags set by the signal
         *          handlers and throws exceptions corresponding to the received
         *          signals. After processing, the flags are reset.
         */
        static void checkSignals();

    private:
        /**
         * @brief Signal handling function.
         * @details This function is called when a registered signal is received.
         *          It sets the corresponding atomic flag to indicate that the
         *          signal has been received.
         *
         * @param signal The received signal (e.g., SIGINT, SIGSEGV).
         */
        static void handleSignal(int signal);

        static std::atomic<bool> mSigintReceived;  /**< Atomic flag for SIGINT signal. */
    }; // SignalHandler
} // FilteringDNSResolver::Utilities

#endif // SIGNAL_HANDLER_HPP

/*** end of file SignalHandler.hpp ***/
