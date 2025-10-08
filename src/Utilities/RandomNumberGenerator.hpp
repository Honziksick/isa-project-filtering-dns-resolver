/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         RandomNumberGenerator.hpp                                     *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    23.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains the declaration of the                     *
 *               `RandomNumberGenerator` class, which provides utility         *
 *               methods for random number generation.                         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file RandomNumberGenerator.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file of the `RandomNumberGenerator` class for random
 *        number generation.
 */

#ifndef RANDOM_NUMBER_GENERATOR_HPP
#define RANDOM_NUMBER_GENERATOR_HPP

#include <random>   // std::random_device, std::mt19937, std::uniform_int_distribution
#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class RandomNumberGenerator
     * @brief Utility class for generating random numbers.
     *
     * @details This class provides static methods and members for generating
     *          random numbers. Within the project it is primarily used to
     *          generate 16-bit DNS Transaction IDs (TXID).
     */
    class RandomNumberGenerator final {
    public:
        /**
         * @brief Generates a random 16-bit DNS Transaction ID.
         *
         * @details This method returns a random  number in the range
         *          of 0 to 65535 (included) and serves as DNS Transaction ID.
         *
         * @return uint16_t A random 16-bit DNS Transaction ID.
         */
        static uint16_t getTxId16();

    private:
        static std::random_device mRandomDevice;  /**< Random device used to seed the random number generator. */
        static std::mt19937 mRandomGenerator;     /**< Mersenne Twister random number generator.               */
        static std::uniform_int_distribution<uint16_t> mTxIdDistribution16;  /**< Distribution for generating 16-bit numbers in range of 0-65535. */
    }; // RandomNumberGenerator
} // FilteringDnsResolver::Utilities

#endif // RANDOM_NUMBER_GENERATOR_HPP

/*** end of file RandomNumberGenerator.hpp ***/
