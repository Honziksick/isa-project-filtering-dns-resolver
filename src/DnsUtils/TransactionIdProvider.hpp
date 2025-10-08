/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         TransactionIdProvider.hpp                                     *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file TransactionIdProvider.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_ID_PROVIDER_HPP
#define DNS_ID_PROVIDER_HPP

#include <cstdint>  // uint16_t
#include <vector>   // std::vector

namespace FilteringDnsResolver::DnsUtils
{
    class TransactionIdProvider final {
    public:
        TransactionIdProvider();

        static constexpr auto ALL_IDS_USED{0};

        [[nodiscard]]
        uint16_t getNextId() noexcept;

        void releaseId(uint16_t id) noexcept;

    private:
        static constexpr auto MAX_RNG_ATTEMPTS{32};

        std::vector<bool> mUsed;

        [[nodiscard]]
        static uint16_t next();

        [[nodiscard]]
        bool isUsed(uint16_t id) noexcept;

        void markUsed(uint16_t id) noexcept;
    }; // TransactionIdProvider
} // FilteringDnsResolver::DnsUtils

#endif // DNS_ID_PROVIDER_HPP

/*** end of file TransactionIdProvider.hpp ***/
