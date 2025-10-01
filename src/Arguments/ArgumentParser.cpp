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
 * CLI11 2.5.0 Copyright (c) 2017-2025 University of Cincinnati, developed by Henry
 * Schreiner under NSF AWARD 1414736. All rights reserved.
 */
#include "Arguments/CLI11.hpp"

using namespace FilteringDNSResolver::Enums;
using namespace FilteringDNSResolver::Constants;
using namespace FilteringDNSResolver::Exceptions;
using namespace std;

namespace FilteringDNSResolver::Arguments
{
    CommandLineOptions ArgumentParser::parseArguments(const int argc, char *argv[]) {
        logger("Starting to parse arguments, argc: %d", argc);

        // Create an instance of CommandLineOptions to hold the parsed options
        CommandLineOptions commandLineOptions;

        // Create an instance of the CLI11 application
        CLI::App app;

        // Set up the CLI11 application
        setupCliApp(app, commandLineOptions);

        // Attempt to parse and validate the arguments
        try {
            app.parse(argc, argv);
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

        logger(
                "Arguments parsed successfully: mResolverServer: %s, mListenPort: %u, "
                "mFilterFilePath: %s, mVerbose: %s,",
                commandLineOptions.mResolverServer.c_str(),
                commandLineOptions.mListenPort,
                commandLineOptions.mFilterFilePath.c_str(),
                commandLineOptions.mVerbose ? "true" : "false");
        logger("Finished parsing arguments");

        return commandLineOptions;
    } // ArgumentParser::parseArguments

    void ArgumentParser::setupCliApp(CLI::App &app, CommandLineOptions &commandLineOptions) {
        // General description of the application
        app.name("dns: Filtering DNS Resolver v1.0");
        app.description(
            "Filtering DNS resolver supports UDP communication protocol and QTYPE=A messages only. "
            "It filters queries for domains listed in a local file (including subdomains). "
            "Allowed queries are forwarded to the specified upstream resolver and responses are relayed back."
        );

        // Customize usage message
        app.usage("   dns -s server [-p port] -f filter_file [-v]");

        // Add options
        app.set_help_flag("-h,--help", "Display this help message and exit with code 0.");

        app.add_option("-s,--server", commandLineOptions.mResolverServer,
                       "Upstream DNS resolver (hostname or IPv4).")
           ->required(true)
           ->expected(1);


        app.add_option("-p,--port", commandLineOptions.mListenPort,
                       "Local UDP port to listen on (default: 53).")
           ->required(false)
           ->expected(0, 1)
           ->check(CLI::Range(CustomLimits::MIN_SERVER_PORT, CustomLimits::MAX_SERVER_PORT));

        app.add_option("-f,--filter-file", commandLineOptions.mFilterFilePath,
                       "Path to the ASCII file with blocked domains (one per line; '#' and empty lines ignored).")
           ->required(true)
           ->expected(1);

        app.add_flag("-v,--verbose", commandLineOptions.mVerbose,
                     "Enable verbose logging to STDERR.");

        // Footer with example usage and error codes
        app.footer(
                "\nEXAMPLE USAGE:\n"
                "  dns -s 1.1.1.1 -f blocked.txt\n"
                "  dns -s resolver.example.org -p 1053 -f /etc/dns/blocked.txt -v\n"
                "\n"
                "\nEXIT CODES:\n"
                "   0  – Success\n"
                "  64  – Invalid argument (usage error)\n"
                "  66  – Filter file not found / unreadable\n"
                "  68  – Invalid resolver hostname\n"
                "  71  – OS/protocol error during startup (socket/bind)\n"
                "  77  – Insufficient privilege (binding privileged port)\n"
                "  78  – Configuration error\n"
                "  110 – Timeout (if used during startup)\n"
                );
    } // ArgumentParser::setupCliApp
} // FilteringDNSResolver::Arguments

/*** end of file ArgumentParser.cpp ***/
