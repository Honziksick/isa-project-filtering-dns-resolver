/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         UdpSockets.cpp                                                *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `UdpSockets` class, which     *
 *               implements UDP socket management for the DNS resolver         *
 *               networking layer. It manages creation, binding, and           *
 *               lifecycle of UDP sockets used for DNS communication,          *
 *               including resolver socket for upstream DNS communication      *
 *               and listener socket for client query reception. The class     *
 *               provides RAII socket management with move semantics and       *
 *               automatic resource cleanup for reliable network operation.    *
 *                                                                             *
 ******************************************************************************/
/**
 * @file UdpSockets.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `UdpSockets` class for UDP socket
 *        management and DNS networking infrastructure functionality.
 */

#include "Networking/UdpSockets.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons(), htonl()
#include <sys/socket.h>  // socket(), setsockopt(), bind()
#include <arpa/inet.h>   // inet_ntoa()
#include <unistd.h>      // close()
#include <cstring>       // std::strerror
#include <cerrno>        // errno()
#include <string>        // std::string
#include <memory>        // std::make_unique

using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::Networking
{
    UdpSockets::UdpSockets(const int resolverSocketFd, const int listenerSocketFd)
        : mResolverSocketFd{resolverSocketFd},
          mListenerSocketFd{listenerSocketFd} {
        logger("UdpSockets constructed with resolverFd=%d, listenerFd=%d",
               resolverSocketFd, listenerSocketFd);
    } // UdpSockets::UdpSockets

    UdpSockets::UdpSockets(UdpSockets &&otherUdpSockets) noexcept
        : mResolverSocketFd(otherUdpSockets.mResolverSocketFd),
          mListenerSocketFd(otherUdpSockets.mListenerSocketFd) {
        logger("UdpSockets move constructor: transferring fds %d, %d",
               mResolverSocketFd, mListenerSocketFd);

        otherUdpSockets.mListenerSocketFd = INVALID_FD;
        otherUdpSockets.mResolverSocketFd = INVALID_FD;
    } // UdpSockets::UdpSockets

    UdpSockets &UdpSockets::operator=(UdpSockets &&otherUdpSockets) noexcept {
        if(this != &otherUdpSockets) {
            logger("UdpSockets move assignment: closing current sockets and transferring");
            closeUdpSockets();
            mResolverSocketFd = otherUdpSockets.mResolverSocketFd;
            mListenerSocketFd = otherUdpSockets.mListenerSocketFd;
            otherUdpSockets.mResolverSocketFd = INVALID_FD;
            otherUdpSockets.mListenerSocketFd = INVALID_FD;
        }
        return *this;
    } // UdpSockets::operator=

    UdpSockets::~UdpSockets() noexcept {
        logger("UdpSockets destructor called - cleaning up sockets");
        closeUdpSockets();
    } // UdpSockets::~UdpSockets

    unique_ptr<UdpSockets> UdpSockets::openUdpSockets(const sockaddr_storage &resolverAddress, const uint16_t listenerPort) {
        logger("UdpSockets::openUdpSockets() called with port %u", listenerPort);
        verbose("Initializing DNS server on port %u", listenerPort);

        // Create listener socket
        logger("Creating listener socket");
        const int listenerSocketFd = createUdpSocket(AF_INET);
        logger("Listener socket creation result: fd=%d", listenerSocketFd);

        if(listenerSocketFd < 0) {
            logger("ERROR: Listener socket creation failed with errno=%d", errno);
            verbose("Failed to create listener socket");
            throw SocketErrorException(
                    "Failed to create UDP socket for listening via "
                    "'socket(AF_INET, SOCK_DGRAM, 0)': " + string(strerror(errno))
                    );
        } // if(listenerSocketFd < 0)

        // Set socket options to allow address reuse
        // SOL_SOCKET - manipulate options at the sockets API level
        // SO_REUSEADDR - allow reuse of local addresses
        logger("Setting socket options for address reuse");
        constexpr int opt = 1;

        if(setsockopt(listenerSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            logger("ERROR: setsockopt failed with errno=%d, closing listener socket", errno);
            close(listenerSocketFd);
            verbose("Failed to configure listener socket");

            throw SocketErrorException(
                    "An error occurred while setting listening socket options via "
                    "'setsockopt(SO_REUSEADDR)': " + string(strerror(errno))
                    );
        } // if(setsockopt() < 0)
        logger("Socket options set successfully");

        // Set the listener socket to the specified port on all interfaces
        logger("Binding listener socket to port %u", listenerPort);
        sockaddr_in listenerAddress{};
        listenerAddress.sin_family = AF_INET;                // IPv4
        listenerAddress.sin_port = htons(listenerPort);      // Set the port to listen on
        listenerAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all interfaces (0.0.0.0)

        // Bind the listener socket
        if(bind(listenerSocketFd, reinterpret_cast<sockaddr*>(&listenerAddress), sizeof(listenerAddress)) < 0) {
            logger("ERROR: bind() failed with errno=%d, closing listener socket", errno);
            close(listenerSocketFd);
            verbose("Failed to bind to port %u - port may already be in use", listenerPort);

            throw SocketErrorException(
                    "Binding the listener socket to port " + to_string(listenerPort) +
                    " via 'bind()' failed: " + string(strerror(errno))
                    );
        } // if(bind() < 0)
        logger("Listener socket bound successfully to 0.0.0.0:%u", listenerPort);

        // Create resolver socket
        logger("Creating resolver socket");
        const int resolverSocketFd = createUdpSocket(resolverAddress.ss_family);
        logger("Resolver socket creation result: fd=%d", resolverSocketFd);

        if(resolverSocketFd < 0) {
            logger("ERROR: Resolver socket creation failed with errno=%d, cleaning up", errno);
            close(listenerSocketFd);
            verbose("Failed to create resolver socket");

            throw SocketErrorException(
                    "Failed to create UDP socket for resolver via "
                    "'socket(AF_INET/AF_INET6, SOCK_DGRAM, 0)': " + string(strerror(errno))
                    );
        } // if(resolverSocketFd < 0)

        logger("Connecting resolver socket to upstream DNS server");
        connectResolverSocket(resolverSocketFd, resolverAddress);
        logger("Upstream DNS resolver connected successfully");

        logger("UDP sockets created successfully: listenerFd=%d, resolverFd=%d",
               listenerSocketFd, resolverSocketFd);
        verbose("DNS server ready on port %u", listenerPort);

        return make_unique<UdpSockets>(resolverSocketFd, listenerSocketFd);
    } // UdpSockets::openUdpSockets

    void UdpSockets::connectResolverSocket(const int resolverSocketFd, const sockaddr_storage &resolverAddress) {
        uint16_t port{0};
        std::string ipString{};

        // IPv4
        if(resolverAddress.ss_family == AF_INET) {
            const auto *addressIPv4 = reinterpret_cast<const sockaddr_in*>(&resolverAddress);
            port = ntohs(addressIPv4->sin_port);
            ipString = inet_ntoa(addressIPv4->sin_addr);

            logger("Connecting resolver socket to IPv4 DNS server %s:%u", ipString.c_str(), port);
            verbose("Establishing connection to IPv4 DNS server %s:%u", ipString.c_str(), port);

            if(connect(resolverSocketFd, reinterpret_cast<const sockaddr*>(addressIPv4), sizeof(sockaddr_in)) < 0) {
                logger("ERROR: connect() failed (IPv4) with errno=%d: %s", errno, strerror(errno));
                verbose("Failed to connect to IPv4 DNS server %s:%u - %s", ipString.c_str(), port, strerror(errno));

                throw SocketErrorException(
                    "Failed to connect resolver socket (IPv4): " + std::string(strerror(errno))
                    );
            }
        }
        // IPv6
        else if(resolverAddress.ss_family == AF_INET6) {
            const auto *addressIPv6 = reinterpret_cast<const sockaddr_in6*>(&resolverAddress);
            port = ntohs(addressIPv6->sin6_port);

            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &addressIPv6->sin6_addr, addressBuffer, sizeof(addressBuffer));
            ipString = addressBuffer;

            logger("Connecting resolver socket to IPv6 DNS server [%s]:%u", ipString.c_str(), port);
            verbose("Establishing connection to IPv6 DNS server [%s]:%u", ipString.c_str(), port);

            if(connect(resolverSocketFd, reinterpret_cast<const sockaddr*>(addressIPv6), sizeof(sockaddr_in6)) < 0) {
                logger("ERROR: connect() failed (IPv6) with errno=%d: %s", errno, strerror(errno));
                verbose("Failed to connect to IPv6 DNS server [%s]:%u - %s", ipString.c_str(), port, strerror(errno));

                throw SocketErrorException(
                    "Failed to connect resolver socket (IPv6): " + std::string(strerror(errno))
                    );
            }
        }
        // Unsupported address family
        else {
            logger("ERROR: Unsupported resolver address family (family=%d)", resolverAddress.ss_family);
            throw SocketErrorException(
                "Unsupported resolver address family (family=" + to_string(resolverAddress.ss_family) + ")"
                );
        }

        logger("Resolver socket successfully connected to DNS server %s:%u", ipString.c_str(), port);
        verbose("Connection to DNS server %s:%u established successfully", ipString.c_str(), port);
    } // UdpSockets::connectResolverSocket

    void UdpSockets::closeUdpSockets() noexcept {
        logger("UdpSockets::closeUdpSockets() called");
        closeResolverSocket();
        closeListenerSocket();
        logger("All sockets closed");
    } // UdpSockets::closeUdpSockets

    int UdpSockets::getResolverSocketFd() const noexcept {
        return mResolverSocketFd;
    } // UdpSockets::getResolverSocketFd

    int UdpSockets::getListenerSocketFd() const noexcept {
        return mListenerSocketFd;
    } // UdpSockets::getListenerSocketFd

    int UdpSockets::createUdpSocket(const int family) {
        logger("Creating UDP socket with socket(%s, SOCK_DGRAM, 0)", family == AF_INET ? "AF_INET" : "AF_INET6");
        const int fd = socket(family, SOCK_DGRAM, 0);
        logger("socket() returned fd=%d", fd);

        return fd;
    } // UdpSockets::createUdpSocket

    void UdpSockets::closeResolverSocket() {
        if(mResolverSocketFd != INVALID_FD) {
            logger("Closing resolver socket fd=%d", mResolverSocketFd);
            close(mResolverSocketFd);
            mResolverSocketFd = INVALID_FD;
        }
    } // UdpSockets::closeResolverSocket

    void UdpSockets::closeListenerSocket() {
        if(mListenerSocketFd != INVALID_FD) {
            logger("Closing listener socket fd=%d", mListenerSocketFd);
            close(mListenerSocketFd);
            mListenerSocketFd = INVALID_FD;
        }
    } // UdpSockets::closeListenerSocket
} // FilteringDnsResolver::Networking

/*** end of file UdpSockets.cpp ***/
