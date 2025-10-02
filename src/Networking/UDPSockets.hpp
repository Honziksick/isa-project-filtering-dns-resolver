/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         UDPSockets.hpp                                                *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/
/**
 * @file UDPSockets.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief
 */

#ifndef UDP_SOCKETS_HPP
#define UDP_SOCKETS_HPP
#include <bits/stdint-uintn.h>

namespace FilteringDNSResolver::Networking
{
    class UdpSockets final {
    public:
        static constexpr int INVALID_FD{-1};

        UdpSockets() = default;

        UdpSockets(const UdpSockets &) = delete;
        UdpSockets &operator=(const UdpSockets &) = delete;

        UdpSockets(UdpSockets &&otherUdpSockets) noexcept;
        UdpSockets &operator=(UdpSockets &&otherUdpSockets) noexcept;

        ~UdpSockets() noexcept;

        static UdpSockets openUdpSockets(uint16_t listenerPort);
        void closeUdpSockets() noexcept;

        [[nodiscard]]
        int getResolverSocketFd() const noexcept;

        [[nodiscard]]
        int getListenerSocketFd() const noexcept;

    private:
        UdpSockets(int resolverSocketFd, int listenerSocketFd);

        static int createUdpSocket();

        void closeResolverSocket();
        void closeListenerSocket();

        int mResolverSocketFd{INVALID_FD};
        int mListenerSocketFd{INVALID_FD};
    }; // UDPSocket
} // FilteringDNSResolver::Networking

#endif // UDP_SOCKETS_HPP

/*** end of file UDPSockets.hpp ***/
