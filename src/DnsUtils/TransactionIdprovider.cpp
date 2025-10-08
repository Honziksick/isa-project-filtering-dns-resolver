/*******************************************************************************
*                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         TransactionIdProvider.cpp                                     *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file TransactionIdProvider.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#include "DnsUtils/TransactionIdProvider.hpp"
#include "Constants/CustomLimits.hpp"
#include "Utilities/RandomNumberGenerator.hpp"
#include <cstdint>  // uint16_t
#include <vector>   // std::vector

using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    TransactionIdProvider::TransactionIdProvider()
        : mUsed{CustomLimits::MAX_TX_ID16 + 1, false} {}

    uint16_t TransactionIdProvider::getNextId() noexcept {
        // First we try a limited number of random attempts
        // (should be sufficient in most cases)
        for(int iAttempt = 0; iAttempt < MAX_RNG_ATTEMPTS; iAttempt++) {
            const uint16_t candidateId = next();
            if(!isUsed(candidateId)) {
                markUsed(candidateId);
                return candidateId;
            }
        }

        // Fallback by linear search (slower)
        // (start from a random position, then wrap around)
        const uint16_t startId = next();
        for(int iStartOffset = 0; iStartOffset <= CustomLimits::MAX_TX_ID16; iStartOffset++) {
            const auto candidateId = static_cast<uint16_t>(startId + iStartOffset);
            if(!isUsed(candidateId)) {
                markUsed(candidateId);
                return candidateId;
            }
        }

        // Oops, all IDs are used
        return ALL_IDS_USED;
    } // TransactionIdProvider::getNextId

    uint16_t TransactionIdProvider::next() {
        return RandomNumberGenerator::getTxId16();
    } // TransactionIdProvider::next

    bool TransactionIdProvider::isUsed(const uint16_t id) noexcept {
        return mUsed[id];
    } // TransactionIdProvider::isUsed

    void TransactionIdProvider::markUsed(const uint16_t id) noexcept {
        mUsed[id] = true;
    } // TransactionIdProvider::markUsed

    void TransactionIdProvider::releaseId(const uint16_t id) noexcept {
        mUsed[id] = false;
    } // TransactionIdProvider::releaseId
}

/*** end of file TransactionIdProvider.cpp ***/
