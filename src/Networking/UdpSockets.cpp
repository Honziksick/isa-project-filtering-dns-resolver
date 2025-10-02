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
 * Description:                                                                *
 *                                                                             *
 ******************************************************************************/

#include "Networking/UdpSockets.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons(), htonl()
#include <sys/socket.h>  // socket(), setsockopt(), bind()
#include <unistd.h>      // close()
#include <cstring>       // std::strerror
#include <string>        // std::string
#include <cerrno>        // errno()

using namespace FilteringDnsResolver::Exceptions;
using namespace std;

namespace FilteringDnsResolver::Networking
{
    UdpSockets::UdpSockets(const int resolverSocketFd, const int listenerSocketFd)
        : mResolverSocketFd{resolverSocketFd},
          mListenerSocketFd{listenerSocketFd} {} // UdpSockets::UdpSockets

    UdpSockets::UdpSockets(UdpSockets &&otherUdpSockets) noexcept
        : mResolverSocketFd(otherUdpSockets.mResolverSocketFd),
          mListenerSocketFd(otherUdpSockets.mListenerSocketFd) {
        otherUdpSockets.mListenerSocketFd = INVALID_FD;
        otherUdpSockets.mResolverSocketFd = INVALID_FD;
    } // UdpSockets::UdpSockets

    UdpSockets &UdpSockets::operator=(UdpSockets &&otherUdpSockets) noexcept {
        if(this != &otherUdpSockets) {
            closeUdpSockets();
            mResolverSocketFd = otherUdpSockets.mResolverSocketFd;
            mListenerSocketFd = otherUdpSockets.mListenerSocketFd;
            otherUdpSockets.mResolverSocketFd = INVALID_FD;
            otherUdpSockets.mListenerSocketFd = INVALID_FD;
        }
        return *this;
    } // UdpSockets::operator=

    UdpSockets::~UdpSockets() noexcept {
        closeUdpSockets();
    } // UdpSockets::~UdpSockets

    UdpSockets UdpSockets::openUdpSockets(const uint16_t listenerPort) {
        // Create listener socket
        const int listenerSocketFd = createUdpSocket();
        if(listenerSocketFd < 0) {
            throw SocketErrorException(
                    "Failed to create UDP socket for listening via "
                    "'socket(AF_INET, SOCK_DGRAM, 0)': " + string(strerror(errno))
                    );
        } // if(listenerSocketFd < 0)

        // Set socket options to allow address reuse
        // SOL_SOCKET - manipulate options at the sockets API level
        // SO_REUSEADDR - allow reuse of local addresses
        int opt = 1;
        if(setsockopt(listenerSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            close(listenerSocketFd);
            throw SocketErrorException(
                    "An error occurred while setting listening socket options via "
                    "'setsockopt(SO_REUSEADDR)': " + string(strerror(errno))
                    );
        } // if(setsockopt() < 0)

        // Set the listener socket to the specified port on all interfaces
        sockaddr_in listenerAddress{};
        listenerAddress.sin_family = AF_INET;                // IPv4
        listenerAddress.sin_port = htons(listenerPort);      // Set the port to listen on
        listenerAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all interfaces (0.0.0.0)

        // Bind the listener socket
        if(bind(listenerSocketFd, reinterpret_cast<sockaddr*>(&listenerAddress), sizeof(listenerAddress)) < 0) {
            close(listenerSocketFd);
            throw SocketErrorException(
                    "Binding the listener socket to port " + to_string(listenerPort) +
                    " via 'bind()' failed: " + string(strerror(errno))
                    );
        } // if(bind() < 0)

        // Create resolver socket
        const int resolverSocketFd = createUdpSocket();
        if(resolverSocketFd < 0) {
            close(listenerSocketFd);
            throw SocketErrorException(
                    "Failed to create UDP socket for resolver via "
                    "'socket(AF_INET, SOCK_DGRAM, 0)': " + string(strerror(errno))
                    );
        } // if(resolverSocketFd < 0)

        logger("UDP sockets ready: 'listenerFd = %d', 'resolverSocketFd = %d'", listenerSocketFd, resolverSocketFd);
        return {resolverSocketFd, listenerSocketFd};
    } // UdpSockets::openUdpSockets

    void UdpSockets::closeUdpSockets() noexcept {
        closeResolverSocket();
        closeListenerSocket();
    } // UdpSockets::closeUdpSockets

    int UdpSockets::getResolverSocketFd() const noexcept {
        return mResolverSocketFd;
    } // UdpSockets::getResolverSocketFd

    int UdpSockets::getListenerSocketFd() const noexcept {
        return mListenerSocketFd;
    } // UdpSockets::getListenerSocketFd

    int UdpSockets::createUdpSocket() {
        return socket(AF_INET, SOCK_DGRAM, 0);
    } // UdpSockets::createUdpSocket

    void UdpSockets::closeResolverSocket() {
        if(mResolverSocketFd != INVALID_FD) {
            close(mResolverSocketFd);
            mResolverSocketFd = INVALID_FD;
        }
    } // UdpSockets::closeResolverSocket

    void UdpSockets::closeListenerSocket() {
        if(mListenerSocketFd != INVALID_FD) {
            close(mListenerSocketFd);
            mListenerSocketFd = INVALID_FD;
        }
    } // UdpSockets::closeListenerSocket
} // FilteringDnsResolver::Networking

/*** end of file UdpSockets.cpp ***/
