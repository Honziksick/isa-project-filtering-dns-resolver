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
 * Description:  This header file provides `DnsQuery` class, which             *
 *               represents a complete parsed DNS query message including      *
 *               header fields, domain name (QNAME), query type (QTYPE) and    *
 *               query class (QCLASS). It extends DnsHeader to provide a       *
 *               comprehensive representation of DNS query data structure      *
 *               for processing and validation purposes.                       *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsQuery.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsQuery` class for complete DNS query
 *        representation and data storage.
 */

#ifndef DNS_QUERY_HPP
#define DNS_QUERY_HPP

#include "DnsUtils/DnsHeader.hpp"
#include <string>  // std::string

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class DnsQuery
     * @brief Represents a complete parsed DNS query message.
     *
     * @details Extends DnsHeader to provide a comprehensive representation
     *          of a DNS query including all essential components: header fields,
     *          domain name (QNAME), query type (QTYPE), and query class (QCLASS).
     *          Also tracks parsing metadata such as question section end offset.
     */
    class DnsQuery final : public DnsHeader {
    public:
        /**
         * @brief Default constructor creating an empty DNS query.
         *
         * @details Initializes all fields to empty/default values. The query
         *          must be populated with actual data before use.
         */
        DnsQuery() = default;

        /**
         * @brief Constructs a DNS query with all essential components.
         *
         * @details Creates a complete DNS query object with header information,
         *          domain name, query type and class. Also stores parsing metadata
         *          for further message processing.
         *
         * @param header Parsed DNS header containing ID, flags, and question count.
         * @param qEndOffset Byte offset where the question section ends in the original message.
         * @param qName Domain name being queried (converted to lowercase).
         * @param qType DNS record type being requested (e.g., A=1, AAAA=28).
         * @param qClass DNS class for the query (typically IN=1).
         */
        explicit DnsQuery(const DnsHeader &header, size_t qEndOffset,
                          std::string qName, uint16_t qType, uint16_t qClass);

        DnsHeader mHeader{};              /**< Complete DNS header with ID, flags, and counts.              */
        size_t mQEndOffset{EMPTY_FIELD};  /**< Byte offset where question section ends in original message. */
        std::string mQName{};             /**< Domain name being queried (lowercase, dot-separated).        */
        uint16_t mQType{EMPTY_FIELD};     /**< DNS record type being requested. */
        uint16_t mQClass{EMPTY_FIELD};    /**< DNS class for the query.         */

        static constexpr auto HEADER_TRUE_SIZE{12};  /**< Size of DNS header in bytes according to RFC 1035. */
        static constexpr auto QTYPE_SIZE{2};         /**< Size of QTYPE field in bytes.  */
        static constexpr auto QCLASS_SIZE{2};        /**< Size of QCLASS field in bytes. */

        static constexpr auto QNAME_COMPRESSION_MASK{0xC0};  /**< Mask to identify compressed QNAME labels. */
    }; // DnsQuery
} // FilteringDnsResolver::DnsUtils

#endif // DNS_QUERY_HPP

/*** end of file DnsQuery.hpp ***/
