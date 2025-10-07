/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsMessenger.hpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    07.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsMessenger.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef DNS_MESSENGER_HPP
#define DNS_MESSENGER_HPP

#include <netinet/in.h> // sockaddr_in
#include <cstdint>      // uint8_t, uint16_t
#include <vector>
#include "DnsUtils/DnsQuery.hpp"
#include "Enums/DnsRCodes.hpp"

namespace FilteringDnsResolver::DnsUtils
{
    class DnsMessenger {

    public:
        explicit DnsMessenger(int listenFd);

        void sendRefusedMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                         const sockaddr_in &clientSourceAddress, const DnsQuery &dnsQuery) const;
        void sendNotImpMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                        const sockaddr_in &clientSourceAddress, const DnsQuery &dnsQuery) const;
        void sendFormErrMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                         const sockaddr_in &clientSourceAddress, const DnsQuery &dnsQuery) const;
        void sendServFailMessage(const uint8_t *pMessageBuffer, size_t messageLength,
                          const sockaddr_in &clientSourceAddress, const DnsQuery &dnsQuery) const;

    private:
        int mListenFd;

        static std::vector<uint8_t> buildErrorReply(const uint8_t *pMessageBuffer,
                                                    size_t messageLength,
                                                    const DnsQuery &parsedQuery,
                                                    Enums::DnsRCodes rcode);

        void sendDnsReply(const std::vector<uint8_t> &dnsReply, const sockaddr_in &destinationAddress) const;

        static void storeBigEndianWordToMessage(std::vector<uint8_t> &messageBuffer, size_t msbIndex, uint16_t valueToStore);
        static uint16_t setResponseFlags(uint16_t flagsFromRequest, Enums::DnsRCodes rcode);
        static size_t getQuestionEndOffset(const uint8_t *messageBuffer, size_t messageLength);

        static constexpr auto INVALID_MESSAGE_LENGTH{0};
        static constexpr uint8_t DNS_LABEL_POINTER_MASK{0xC0};
    }; // DnsMessenger
} // FilteringDnsResolver::DnsUtils

#endif // DNS_MESSENGER_HPP

/*** end of file DnsMessenger.hpp ***/
