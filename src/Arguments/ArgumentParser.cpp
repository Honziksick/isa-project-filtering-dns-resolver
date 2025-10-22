/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ArgumentParser.cpp                                            *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    25.09.2025                                                    *
 *                                                                             *
 * Description:  Implementation of the `ArgumentParser` class, which is        *
 *               responsible for parsing command line arguments and options.   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ArgumentParser.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of the `ArgumentParser` class for parsing command line
 *        arguments and options.
 */

#include "Arguments/ArgumentParser.hpp"
#include "Arguments/CommandLineOptions.hpp"
#include "Constants/CustomLimits.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <string>  // std::string
#include <regex>   // std::regex

/* CLI11 je header-only library for command-line parsing
 * Source: https://github.com/CLIUtils/CLI11
 * License: See below (for more details, refer to the "CLI11.hpp" file).
 * CLI11 2.5.0 Copyright (c) 2017-2025 University of Cincinnati, developed by
 * Henry Schreiner under NSF AWARD 1414736. All rights reserved.
 */
#include "Arguments/CLI11.hpp"

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::Arguments
{
    CommandLineOptions ArgumentParser::parseArguments(const int argc, char *argv[]) {
        logger("ArgumentParser::parseArguments() called with argc=%d", argc);
        verbose("Parsing command line arguments...");
        for(int iArgument = 0; iArgument < argc; iArgument++) {
            logger("argv[%d]: %s", iArgument, argv[iArgument]);
        }

        // Create an instance of CommandLineOptions to hold the parsed options
        logger("Creating CommandLineOptions instance with default values");
        CommandLineOptions commandLineOptions{};

        // Create an instance of the CLI11 application
        logger("Initializing CLI11 application parser");
        CLI::App app;

        // Set up the CLI11 application
        logger("Setting up CLI11 application configuration");
        setupCliApp(app, commandLineOptions);
        logger("CLI11 application setup completed");

        // Attempt to parse and validate the arguments
        logger("Beginning CLI11 argument parsing");
        try {
            app.parse(argc, argv);
            logger("CLI11 parsing completed successfully");
        }
        catch(const CLI::CallForHelp &e) {
            if(const auto exitCodes{app.exit(e)}; exitCodes == EXIT_SUCCESS) {
                throw HelpRequestedException();
            }
            else {
                throw InternalErrorException(
                        "CLI11 library returned error while parsing "
                        "command line arguments: " + string(e.what())
                        );
            }
        }
        catch(const CLI::Error &e) {
            throw InvalidArgumentException(string(e.what()));
        }

        logger("Arguments parsed successfully: mResolverHostname='%s', mListenPort=%u, "
               "mFilterFilePath='%s', mVerbose=%s",
               commandLineOptions.mResolverHostname.c_str(),
               commandLineOptions.mListenPort,
               commandLineOptions.mFilterFilePath.c_str(),
               commandLineOptions.mVerbose ? "true" : "false");

        verbose("Configuration: upstream=%s, port=%u, filter=%s%s",
                commandLineOptions.mResolverHostname.c_str(),
                commandLineOptions.mListenPort,
                commandLineOptions.mFilterFilePath.c_str(),
                commandLineOptions.mVerbose ? ", verbose=on" : "");

        logger("ArgumentParser::parseArguments() completed successfully");

        return commandLineOptions;
    } // ArgumentParser::parseArguments

    void ArgumentParser::setupCliApp(CLI::App &app, CommandLineOptions &commandLineOptions) {
        logger("ArgumentParser::setupCliApp() started");

        // General description of the application
        logger("Setting CLI11 application name and description");
        app.name("dns: Filtering DNS Resolver v1.0");
        app.description(
                "Filtering DNS resolver supports UDP communication protocol and QTYPE=A messages only. "
                "It filters queries for domains listed in a local file (including subdomains). "
                "Allowed queries are forwarded to the specified upstream resolver and responses are relayed back."
                );

        // Customize usage message
        logger("Setting CLI11 custom usage message");
        app.usage("   dns -s server [-p port] -f filter_file [-v]");

        // Add options
        logger("Configuring CLI11 help flag");
        app.set_help_flag("-h,--help", "Display this help message and exit with code 0.");

        logger("Adding required server option with validation");
        app.add_option("-s,--server", commandLineOptions.mResolverHostname,
                       "Upstream DNS resolver (hostname or IPv4).")
           ->required(true)
           ->expected(1);

        logger("Adding optional port option with range validation [%d-%d]",
               CustomLimits::MIN_SERVER_PORT, CustomLimits::MAX_SERVER_PORT);
        app.add_option("-p,--port", commandLineOptions.mListenPort,
                       "Local UDP port to listen on (default: 53).")
           ->required(false)
           ->expected(0, 1)
           ->check(CLI::Range(CustomLimits::MIN_SERVER_PORT, CustomLimits::MAX_SERVER_PORT));

        logger("Adding required filter file option");
        app.add_option("-f,--filter", commandLineOptions.mFilterFilePath,
                       "Path to the ASCII file with blocked domains (one per line; '#' and empty lines ignored).")
           ->required(true)
           ->expected(1);

        logger("Adding verbose flag option");
        app.add_flag("-v,--verbose", commandLineOptions.mVerbose,
                     "Enable verbose logging to STDERR.");

        // Footer with example usage and error codes
        logger("Setting CLI11 application footer with usage examples and exit codes");
        app.footer(
                "\nEXAMPLE USAGE:\n"
                "  dns -s 1.1.1.1 -f blocked.txt\n"
                "  dns -s resolver.example.org -p 1053 -f blocked.txt -v\n"
                "\n"
                "\nEXIT CODES:\n"
                "   0  – Success\n"
                "   1  – Internal error\n"
                "  22  – Invalid argument (usage error)\n"
                "  23  – Invalid filter file content\n"
                "  42  – Unknown error\n"
                "  71  – OS/protocol error during startup (socket/bind)\n"
                " 107  – Socket error\n"
                " 111  – Connection error (send / receive)\n"
                " 113  – Hostname resolution error (getaddrinfo)\n"
                );

        logger("ArgumentParser::setupCliApp() completed successfully");
    } // ArgumentParser::setupCliApp
} // FilteringDnsResolver::Arguments

/*** end of file ArgumentParser.cpp ***/
