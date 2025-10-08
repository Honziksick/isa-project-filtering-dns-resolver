/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         RandomNumberGenerator.cpp                                     *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      23.09.2025                                                    *
 * Last edit:    23.09.2025                                                    *
 *                                                                             *
 * Description:  This file contains the implementation of the                  *
 *               `RandomNumberGenerator` class, which provides utility         *
 *               methods for random number generation.                         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file RandomNumberGenerator.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Implementation of the `RandomNumberGenerator` class for random
 *        number generation.
 */

#include "Utilities/RandomNumberGenerator.hpp"
#include "Constants/CustomLimits.hpp"

using namespace FilteringDnsResolver::Constants;
using namespace std;

namespace FilteringDnsResolver::Utilities
{
    // Inicialize the static members of the RandomNumberGenerator class
    random_device RandomNumberGenerator::mRandomDevice;
    mt19937 RandomNumberGenerator::mRandomGenerator{mRandomDevice()};
    uniform_int_distribution<uint16_t> RandomNumberGenerator::mTxIdDistribution16(CustomLimits::MIN_TX_ID16,
                                                                                  CustomLimits::MAX_TX_ID16); // allowed DNS TX ID range

    uint16_t RandomNumberGenerator::getTxId16() {
        return mTxIdDistribution16(mRandomGenerator);
    } // RandomNumberGenerator::getTxId16()
} // FilteringDnsResolver::Utilities

/*** end of file RandomNumberGenerator.hpp ***/
