/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DomainFilter.hpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      01.10.2025                                                    *
 * Last edit:    01.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DomainFilter` class, which         *
 *               implements efficient domain name filtering for the DNS        *
 *               resolver. It provides fast domain matching using hash-based   *
 *               lookups with heterogeneous access support for `string_view`   *
 *               queries. The filter supports subdomain matching and uses      *
 *               optimized data structures for high-performance DNS query      *
 *               filtering in real-time network applications.                  *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainFilter.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DomainFilter` class for efficient domain
 *        name filtering and DNS query matching functionality.
 *
 * @note This implementation was inspired by:
 *       https://www.cppstories.com/2021/heterogeneous-access-cpp20/
 *       and
 *       https://ebadblog.com/looking-up-a-c++-hash-table-with-a-pre-known-hash
 */

#ifndef DOMAIN_FILTER_HPP
#define DOMAIN_FILTER_HPP

#include <unordered_set>
#include <string_view>
#include <string>
#include <vector>

namespace FilteringDnsResolver::Filter
{
    /**
     * @class DomainFilter
     * @brief Efficient domain name filtering engine for DNS query processing.
     *
     * @details Provides fast domain matching using hash-based lookups with
     *          heterogeneous access support. Supports exact domain matching
     *          and subdomain filtering for comprehensive DNS query filtering
     *          with optimized performance for real-time applications.
     */
    class DomainFilter {
    public:
        /**
         * @brief Constructs domain filter from configuration file content.
         *
         * @details Initializes the filter with domain names from parsed
         *          configuration file. Builds optimized hash set for fast
         *          domain matching during DNS query processing.
         *
         * @param filterFileContent Vector of domain names from filter configuration.
         */
        explicit DomainFilter(std::vector<std::string> filterFileContent);

        /**
         * @brief Checks if domain name matches any filtered domain.
         *
         * @details Performs efficient hash-based lookup to determine if the
         *          given domain name or any of its parent domains matches
         *          the configured filter rules.
         *
         * @param domainName Domain name to check against filter rules.
         *
         * @return `true` if domain matches filter, `false` otherwise.
         */
        bool domainMatches(std::string_view domainName) const;

    private:
        /**
         * @struct StringHash
         * @brief Transparent hash functor for heterogeneous string lookups.
         *
         * @details Enables efficient hash set lookups using string_view
         *          without temporary string allocation, improving performance
         *          for domain matching operations.
         *
         * @note Implemetation of `StringHash` and its usage was inspired by:
         *       https://ebadblog.com/looking-up-a-c++-hash-table-with-a-pre-known-hash
         */
        struct StringHash {
            using is_transparent = void;  /**< Enables heterogeneous lookup support */

            /**
             * @brief Computes hash value for string_view input.
             *
             * @details Provides transparent hashing for string_view objects
             *          to enable efficient lookups without string conversion.
             *
             * @param sv String view to hash.
             *
             * @return Hash value for the string view.
             */
            std::size_t operator()(std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            } // operator()
        }; // StringHash

        using DomainNameSet = std::unordered_set<std::string, StringHash, std::equal_to<>>;  /**< Hash set type for domain storage with transparent access */

        DomainNameSet mDomainNameSet; /**< Hash set storing filtered domain names for fast lookup */

        static constexpr auto DOT{'.'}; /**< Domain separator character for subdomain processing */
    }; // DomainFilter
} // FilteringDnsResolver::Filter

#endif // DOMAIN_FILTER_HPP

/*** end of file DomainFilter.hpp ***/
