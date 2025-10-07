/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeader.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeader.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_HEADER_HPP
#define DNS_HEADER_HPP

#include "Constants/DnsHeaderFlagMasks.hpp"
#include "Constants/DnsHeaderIndexes.hpp"
#include "Enums/DnsRCodes.hpp"
#include <cstdint>  // uint16_t

// https://medium.com/@s12deff/command-and-control-c2-dns-server-part-i-d662a6764aff

namespace FilteringDnsResolver::DnsUtils
{
    class DnsHeader : public Constants::DnsHeaderIndexes, public Constants::DnsHeaderFlagMasks {
    public:
        DnsHeader() = default;

        explicit DnsHeader(uint16_t id, uint16_t flags, uint16_t qdCount);

        uint16_t mId{EMPTY_FIELD};
        uint16_t mFlags{EMPTY_FIELD};
        uint16_t mQdCount{EMPTY_FIELD};

        [[nodiscard]]
        static bool isQRSet(uint16_t flags);

        [[nodiscard]]
        static bool isRDSet(uint16_t flags);

        [[nodiscard]]
        static uint16_t getOpcode(uint16_t flags);

        [[nodiscard]]
        static bool isZBitZero(uint16_t flags);

        static constexpr uint16_t EMPTY_FIELD{0};

    private:
        using enum Enums::DnsRCodes;
        static constexpr int OPCODE_SHIFT{11};
    }; // DnsHeader
} // FilteringDnsResolver::DNS

#endif // DNS_HEADER_HPP

/*** end of file DnsHeader.hpp ***/
