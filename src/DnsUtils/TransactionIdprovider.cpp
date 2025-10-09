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
 * Description:  This source file implements `TransactionIdProvider` class,    *
 *               which manages allocation and deallocation of DNS transaction  *
 *               IDs for the resolver. It provides thread-safe generation of   *
 *               unique 16-bit transaction IDs using random number generation  *
 *               and bitmap tracking to prevent ID conflicts in concurrent     *
 *               DNS query processing.                                         *
 *                                                                             *
 ******************************************************************************/
/**
 * @file TransactionIdProvider.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `TransactionIdProvider` class for
 *        DNS transaction ID allocation and management functionality.
 */

#include "DnsUtils/TransactionIdProvider.hpp"
#include "Constants/CustomLimits.hpp"
#include "Utilities/RandomNumberGenerator.hpp"
#include "Utilities/Logger.hpp"
#include <cstdint>  // uint16_t
#include <vector>   // std::vector

using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    TransactionIdProvider::TransactionIdProvider()
        : mUsed(CustomLimits::MAX_TX_ID16 + 1, false) {
        logger("TransactionIdProvider::TransactionIdProvider() constructor called");
        logger("Initialized bitmap for %u transaction IDs (0-%u)",
               CustomLimits::MAX_TX_ID16 + 1, CustomLimits::MAX_TX_ID16);
        logger("TransactionIdProvider object constructed at %p", static_cast<void*>(this));
    }

    uint16_t TransactionIdProvider::getNextId() noexcept {
        logger("TransactionIdProvider::getNextId() called - searching for available ID");

        // First we try a limited number of random attempts
        // (should be sufficient in most cases)
        logger("Starting random search phase (%d attempts)", MAX_RNG_ATTEMPTS);
        for(int iAttempt = 0; iAttempt < MAX_RNG_ATTEMPTS; iAttempt++) {
            const uint16_t candidateId = next();
            logger("Random attempt %d: generated ID=%u (0x%04X)", iAttempt + 1, candidateId, candidateId);

            if(!isUsed(candidateId)) {
                markUsed(candidateId);
                logger("Random TX ID search successful: allocated ID=%u after %d attempts", candidateId, iAttempt + 1);
                verbose("Allocated transaction ID %u for new DNS query", candidateId);
                return candidateId;
            }
            logger("ID=%u already in use, trying next random candidate", candidateId);
        } // for(RNG attempts)

        logger("Random search exhausted after %d attempts - falling back to linear search", MAX_RNG_ATTEMPTS);
        verbose("High transaction ID usage - performing comprehensive search");

        // Fallback by linear search (slower)
        // (start from a random position, then wrap around)
        const uint16_t startId = next();
        logger("Linear search starting from random position: %u (0x%04X)", startId, startId);
        for(int iStartOffset = 0; iStartOffset <= CustomLimits::MAX_TX_ID16; iStartOffset++) {
            const auto candidateId = static_cast<uint16_t>(startId + iStartOffset);
            logger("Linear search attempt %d: checking ID=%u (0x%04X)",
       iStartOffset + 1, candidateId, candidateId);

            if(!isUsed(candidateId)) {
                markUsed(candidateId);
                logger("Linear search successful: allocated ID=%u after checking %d positions",
                       candidateId, iStartOffset + 1);
                verbose("Allocated transaction ID %u after comprehensive search", candidateId);
                return candidateId;
            }
        } // for(linear search)

        // Oops, all IDs are used
        logger("Critical: All %u transaction IDs are exhausted - returning ALL_IDS_USED",
               CustomLimits::MAX_TX_ID16 + 1);
        verbose("Warning: No available transaction IDs - DNS query processing may be limited");
        return ALL_IDS_USED;
    } // TransactionIdProvider::getNextId

    uint16_t TransactionIdProvider::next() {
        const uint16_t randomId = RandomNumberGenerator::getTxId16();
        logger("RandomNumberGenerator::getTxId16() returned: %u (0x%04X)", randomId, randomId);
        return randomId;
    } // TransactionIdProvider::next

    bool TransactionIdProvider::isUsed(const uint16_t id) noexcept {
        const bool used = mUsed[id];
        logger("Checking ID=%u usage: %s", id, used ? "USED" : "AVAILABLE");
        return used;
    } // TransactionIdProvider::isUsed

    void TransactionIdProvider::markUsed(const uint16_t id) noexcept {
        logger("Marking ID=%u as USED in bitmap", id);
        mUsed[id] = true;
        logger("ID=%u successfully marked as used", id);
    } // TransactionIdProvider::markUsed

    void TransactionIdProvider::releaseId(const uint16_t id) noexcept {
        logger("TransactionIdProvider::releaseId() called for ID=%u (0x%04X)", id, id);

        if(!mUsed[id]) {
            logger("Warning: attempting to release already available ID=%u", id);
        } else {
            logger("Releasing ID=%u from used state", id);
        }

        mUsed[id] = false;
        logger("ID=%u successfully released and marked as available", id);
        verbose("Released transaction ID %u - now available for reuse", id);
    } // TransactionIdProvider::releaseId
} // FilteringDnsResolver::DnsUtils

/*** end of file TransactionIdProvider.cpp ***/
