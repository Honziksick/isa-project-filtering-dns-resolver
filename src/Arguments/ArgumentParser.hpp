/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ArgumentParser.hpp                                            *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    25.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains the definition of the `ArgumentParser`     *
 *               class, which is responsible for parsing command line          *
 *               arguments and options for the Filtering DNS Resolver.         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ArgumentParser.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file for the `ArgumentParser` class for parsing command
 *        line arguments.
 */

#ifndef COMMAND_LINE_PARSER_HPP
#define COMMAND_LINE_PARSER_HPP

#include "Arguments/CommandLineOptions.hpp"

/* CLI11 je header-only library for command-line parsing
 * Source: https://github.com/CLIUtils/CLI11
 * License: See below (for more details, refer to the "CLI11.hpp" file).
 * CLI11 2.5.0 Copyright (c) 2017-2025 University of Cincinnati, developed by
 * Henry Schreiner under NSF AWARD 1414736. All rights reserved.
 */
#include "Arguments/CLI11.hpp"

namespace FilteringDnsResolver::Arguments
{
    /**
     * @class ArgumentParser
     * @brief Class responsible for parsing command line arguments and options.
     */
    class ArgumentParser final {
    public:
        /**
         * @brief Parses command line arguments and returns a populated
         *        CommandLineOptions object.
         *
         * @param argc Number of arguments.
         * @param argv Array of argument strings.
         * @return `CommandLineOptions` instance initialized with parsed values.
         */
        static CommandLineOptions parseArguments(int argc, char *argv[]);

    private:
        /**
         * @brief Sets up the CLI application with the necessary options and
         *        arguments.
         *
         * @param app Reference to the CLI application instance.
         * @param commandLineOptions Reference to the `CommandLineOptions` object to be populated.
         */
        static void setupCliApp(CLI::App &app, CommandLineOptions &commandLineOptions);
    }; // ArgumentParser
} // FilteringDnsResolver::Arguments

#endif // COMMAND_LINE_PARSER_HPP

/*** end of file ArgumentParser.hpp ***/
