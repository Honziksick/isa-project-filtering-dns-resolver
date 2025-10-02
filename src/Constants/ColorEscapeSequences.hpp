/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ColorEscapeSequences.hpp                                      *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    24.09.2025                                                    *
 *                                                                             *
 * Description: This file contains the definition of color escape sequences    *
 *              used for formatting text output in the Filtering DNS Resolver  *
 *              project.                                                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ColorEscapeSequences.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief This file defines constants for color escape sequences used in
 *        text/output formatting.
 */

#ifndef COLOR_ESCAPE_SEQUENCES_HPP
#define COLOR_ESCAPE_SEQUENCES_HPP

namespace FilteringDnsResolver::Constants
{
    /**
     * @class Color
     * @brief Class containing constants for color escape sequences used in
     *        text formatting.
     */
    class Color {
    public:
        // General formatting escape sequences
        static constexpr auto RESET = "\033[0m";        /**< Reset all attributes. */
        static constexpr auto FORMAT_BOLD = "\033[1m";  /**< Bold text format.     */

        // Foreground colors
        static constexpr auto RED = "\033[31m";      /**< Red text color.     */
        static constexpr auto GREEN = "\033[32m";    /**< Green text color.   */
        static constexpr auto YELLOW = "\033[33m";   /**< Yellow text color.  */
        static constexpr auto MAGENTA = "\033[35m";  /**< Magenta text color. */
        static constexpr auto CYAN = "\033[36m";     /**< Cyan text color.    */
    }; // Color
} // FilteringDnsResolver::Constants

#endif // COLOR_ESCAPE_SEQUENCES_HPP

/*** end of file ColorEscapeSequences.hpp ***/
