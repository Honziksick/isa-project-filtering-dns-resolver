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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DomainFilter.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

// https://www.cppstories.com/2021/heterogeneous-access-cpp20/

#ifndef DOMAIN_FILTER_HPP
#define DOMAIN_FILTER_HPP

#include <unordered_set>
#include <string_view>
#include <string>
#include <vector>

namespace FilteringDnsResolver::Filter
{
    class DomainFilter {
    public:
        explicit DomainFilter(std::vector<std::string> filterFileContent);

        bool domainMatches(std::string_view domainName) const;

    private:
        // https://ebadblog.com/looking-up-a-c++-hash-table-with-a-pre-known-hash
        struct StringHash {
            using is_transparent = void;

            std::size_t operator()(std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            }
        };

        using DomainNameSet = std::unordered_set<std::string, StringHash, std::equal_to<>>;

        DomainNameSet mDomainNameSet;

        static constexpr auto DOT{'.'};
    }; // DomainFilter
} // FilteringDnsResolver::Filter

#endif // DOMAIN_FILTER_HPP

/*** end of file DomainFilter.hpp ***/
