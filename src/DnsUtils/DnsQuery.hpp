/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsQuery.hpp                                                  *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsQuery.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_QUERY_HPP
#define DNS_QUERY_HPP

#include "DnsUtils/DnsHeader.hpp"
#include <string>  // std::string

namespace FilteringDnsResolver::DnsUtils
{
    class DnsQuery final : public DnsHeader {
    public:
        DnsQuery() = default;

        explicit DnsQuery(const DnsHeader &header, size_t qEndOffset,
                          std::string qName, uint16_t qType, uint16_t qClass);

        DnsHeader mHeader{};
        size_t mQEndOffset{EMPTY_FIELD};
        std::string mQName{};
        uint16_t mQType{EMPTY_FIELD};
        uint16_t mQClass{EMPTY_FIELD};

        static constexpr auto HEADER_TRUE_SIZE{12};
        static constexpr auto QTYPE_SIZE{2};
        static constexpr auto QCLASS_SIZE{2};
    }; // DnsQuery
} // FilteringDnsResolver::DnsUtils

#endif // DNS_QUERY_HPP

/*** end of file DnsQuery.hpp ***/
