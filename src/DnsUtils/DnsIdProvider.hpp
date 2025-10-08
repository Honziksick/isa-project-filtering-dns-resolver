/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsIdProvider.hpp                                             *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsIdProvider.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_ID_PROVIDER_HPP
#define DNS_ID_PROVIDER_HPP

#include "Constants/CustomLimits.hpp"
#include "Utilities/RandomNumberGenerator.hpp"
#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::DnsUtils
{
    class DnsIdProvider final {
    public:
        DnsIdProvider() = default;

        static constexpr auto ALL_IDS_USED{0};

        template <class IsUsed>
        [[nodiscard]]
        uint16_t getNextId(IsUsed isUsed) const {
            // First we try a limited number of random attempts
            // (should be sufficient in most cases)
            for(int iAttempt = 0; iAttempt < MAX_RNG_ATTEMPTS; iAttempt++) {
                const uint16_t candidateId = next();
                if(!isUsed(candidateId)) {
                    return candidateId;
                }
            }

            // Fallback by linear search (slower)
            // (start from a random position, then wrap around)
            const uint16_t startId = next();
            for(int iStartOffset = 0; iStartOffset <= Constants::CustomLimits::MAX_TX_ID16; iStartOffset++) {
                const auto candidateId = static_cast<uint16_t>(startId + iStartOffset);
                if(!isUsed(candidateId)) {
                    return candidateId;
                }
            }

            // Oops, all IDs are used
            return ALL_IDS_USED;
        }

    private:
        static constexpr auto MAX_RNG_ATTEMPTS{32};

        [[nodiscard]]
        static uint16_t next() {
            return Utilities::RandomNumberGenerator::getTxId16();
        }
    }; // DnsIdProvider
} // FilteringDnsResolver::DnsUtils

#endif // DNS_ID_PROVIDER_HPP

/*** end of file DnsIdProvider.hpp ***/
