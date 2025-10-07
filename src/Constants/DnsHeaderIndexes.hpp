/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeaderIndexes.hpp                                          *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeaderIndexes.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_HEADER_INDEXES_HPP
#define DNS_HEADER_INDEXES_HPP

namespace FilteringDnsResolver::Constants
{
    class DnsHeaderIndexes {
    public:
        static constexpr auto ID_MSB{0};
        static constexpr auto ID_LSB{1};
        static constexpr auto FLAGS_MSB{2};
        static constexpr auto FLAGS_LSB{3};
        static constexpr auto QDCOUNT_MSB{4};
        static constexpr auto QDCOUNT_LSB{5};
        static constexpr auto ANCOUNT_MSB{6};
        static constexpr auto ANCOUNT_LSB{7};
        static constexpr auto NSCOUNT_MSB{8};
        static constexpr auto NSCOUNT_LSB{9};
        static constexpr auto ARCOUNT_MSB{10};
        static constexpr auto ARCOUNT_LSB{11};
    }; // DnsHeaderIndexes
} // FilteringDnsResolver::Constants

#endif // DNS_HEADER_INDEXES_HPP

/*** end of file DnsHeaderIndexes.hpp ***/
