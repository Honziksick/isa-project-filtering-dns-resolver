/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         Logger.hpp                                                    *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    09.10.2025                                                    *
 *                                                                             *
 * Description:  Header file for the `Logger` utility, which provides a        *
 *               flexible mechanism for logging debug messages with contextual *
 *               information such as file name line number, and function name. *
 *               Implemented back-compatible with C++17.                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file Logger.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief  Header file for the Logger utility, which provides a flexible
 *         mechanism for logging debug messages
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "Constants/ColorEscapeSequences.hpp"
#include <string_view>  // std::string_view
#include <sstream>      // std::stringstream
#include <iostream>     // std::cerr, std::endl
#include <iomanip>      // std::setw
#include <utility>      // std::forward
#include <mutex>        // std::mutex
#include <cstdio>       // std::fprintf()
#include <cstring>      // std::strstr()

// Define DEBUG_PRINT to enable debug printing
// #define DEBUG_PRINT

/**
 * @brief Global flag controlling verbose output mode.
 *
 * @details This boolean variable determines whether verbose messages
 *          are displayed. It should be set to true when the user
 *          provides the -v or --verbose command line argument.
 *          Default value is `false` (verbose mode disabled).
 */
extern bool gIsVerboseSet;

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class Logger
     * @brief Provides a thread-safe logging mechanism for debug messages.
     *
     * @details The Logger class allows logging messages with contextual
     *          information such as the file name, line number, and function
     *          name. It uses a mutex to ensure thread safety and supports
     *          printf-style formatting for log messages.
     */
    class Logger {
    private:
        static inline std::mutex mLogMutex{};  /**< Mutex to ensure thread-safe logging. */

    public:
        /**
         * @brief Logs a message with contextual information.
         * @details This method logs a message to the standard error stream
         *          (`stderr`) with contextual information. If additional
         *          arguments are provided, they are formatted into the message
         *          using the format string.
         *
         * @tparam Args Variadic template for additional arguments to format the message.
         *
         * @param file The name of the source file where the log is called.
         * @param line The line number in the source file where the log is called.
         * @param func The name of the function where the log is called.
         * @param format A printf-style format string for the log message.
         * @param color The color escape sequence to use for the log message.
         * @param args Additional arguments for the format string.
         */
        template <typename ... Args>
        static void log(const char *file, const int line, const char *func,
                        const std::string_view format, const char *color, Args && ... args) {
            // Thread-safety
            std::lock_guard<std::mutex> lock(mLogMutex);

            // Shorten the file path, so it starts in project root
            const char *shortPath = file;
            if(const char *srcPosition = std::strstr(file, "/src/"); srcPosition != nullptr) {
                shortPath = srcPosition + 1;  // +1, so it looks like "src/" and not "/src/"
            }

            // Format the log message
            std::stringstream logMessage;
            logMessage << color
                    << std::left << std::setw(35) << shortPath << ":"
                    << std::left << std::setw(4) << line << " | "
                    << std::right << std::setw(30) << func << " | ";

            // Print the message
            if constexpr(sizeof...(args) > 0) {
                fprintf(stderr, "%s", logMessage.str().c_str());
                fprintf(stderr, format.data(), std::forward<Args>(args) ...);
                fprintf(stderr, "%s\n", Constants::Color::RESET);
                fflush(stderr);
            }
            else {
                logMessage << format << Constants::Color::RESET;
                std::cerr << logMessage.str() << std::endl;
                std::cerr << std::flush;
            }
        } // Logger::log()
    }; // Logger
} // FilteringDnsResolver::Utilities

/**
 * @def logger(format, ...)
 * @brief Macro for conditional debug logging.
 *
 * @details This macro logs messages to the standard error stream (`stderr`)
 *          with contextual information such as the file name, line number,
 *          and function name. The output is color-coded using escape sequences
 *          defined in `ColorEscapeSequences.hpp`.
 *
 * @note If `DEBUG_PRINT` is not defined, the macro does nothing.
 *
 * @param format A printf-style format string for the log message.
 * @param ... Additional arguments for the format string.
 *
 */
#ifdef DEBUG_PRINT
#define logger(format, ...) \
FilteringDnsResolver::Utilities::Logger::log(__FILE__, __LINE__, __func__, format, FilteringDnsResolver::Constants::Color::MAGENTA, ##__VA_ARGS__)
#else
#define logger(format, ...) (0)
#endif // DEBUG_PRINT

/**
* @def verbose(format, ...)
 * @brief Macro for conditional verbose logging.
 *
 * @details This macro logs verbose messages to the standard error stream (`stderr`)
 *          with contextual information such as the file name, line number,
 *          and function name. The output is color-coded using cyan escape sequences
 *          defined in `ColorEscapeSequences.hpp`. Verbose logging provides detailed
 *          information for debugging and troubleshooting purposes.
 *
 * @note The macro only outputs messages if the global variable `gIsVerboseSet`
 *       is set to true (typically controlled by the -v command line argument).
 *       This allows runtime control of verbose output without recompilation.
 *
 * @param format A printf-style format string for the verbose log message.
 * @param ... Additional arguments for the format string.
 */
#define verbose(format, ...) do { \
if (gIsVerboseSet) { \
FilteringDnsResolver::Utilities::Logger::log(__FILE__, __LINE__, __func__, format, FilteringDnsResolver::Constants::Color::CYAN, ##__VA_ARGS__); \
} \
} while(0)

#endif // LOGGER_HPP

/*** end of file Logger.hpp ***/
