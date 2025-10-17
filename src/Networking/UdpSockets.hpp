/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         UdpSockets.hpp                                                *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    14.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `UdpSockets` class, which           *
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
 * @file UdpSockets.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `UdpSockets` class for UDP socket management
 *        and DNS networking infrastructure functionality.
 */

#ifndef UDP_SOCKETS_HPP
#define UDP_SOCKETS_HPP

#include <netdb.h>  // sockaddr_in
#include <cstdint>  // uint16_t
#include <memory>   // std::unique_ptr

namespace FilteringDnsResolver::Networking
{
    /**
     * @class UdpSockets
     * @brief RAII UDP socket manager for DNS resolver networking operations.
     *
     * @details Manages UDP socket creation, binding, and lifecycle for DNS
     *          communication infrastructure. Provides resolver socket for
     *          upstream DNS server communication and listener socket for
     *          client query reception with automatic resource management
     *          and move semantics for efficient socket handling.
     */
    class UdpSockets final {
    public:
        /**
         * @brief Default constructor creates invalid socket pair.
         *
         * @details Initializes socket manager with invalid file descriptors,
         *          requiring explicit socket creation through openUdpSockets().
         */
        UdpSockets() = default;

        /**
         * @brief Private constructor for internal socket pair initialization.
         *
         * @details Creates socket manager with provided file descriptors
         *          for internal use by factory methods and move operations.
         *
         * @param resolverSocketFd File descriptor for resolver socket.
         * @param listenerSocketFd File descriptor for listener socket.
         */
        UdpSockets(int resolverSocketFd, int listenerSocketFd);

        UdpSockets(const UdpSockets &) = delete;            /**< Copy constructor deleted for exclusive socket ownership. */
        UdpSockets &operator=(const UdpSockets &) = delete; /**< Copy assignment deleted for exclusive socket ownership.  */

        static constexpr int INVALID_FD{-1}; /**< Invalid file descriptor constant for socket error states. */

        /**
         * @brief Move constructor transfers socket ownership.
         *
         * @details Moves socket file descriptors from source object and
         *          invalidates source sockets to maintain exclusive ownership.
         *
         * @param otherUdpSockets Source socket manager for ownership transfer.
         */
        UdpSockets(UdpSockets &&otherUdpSockets) noexcept;

        /**
         * @brief Move assignment operator transfers socket ownership.
         *
         * @details Closes current sockets and transfers ownership from source
         *          object while invalidating source socket descriptors.
         *
         * @param otherUdpSockets Source socket manager for ownership transfer.
         *
         * @return Reference to this socket manager after ownership transfer.
         */
        UdpSockets &operator=(UdpSockets &&otherUdpSockets) noexcept;

        /**
         * @brief Destructor automatically closes managed sockets.
         *
         * @details Ensures proper socket cleanup and resource deallocation
         *          when socket manager goes out of scope or is destroyed.
         */
        ~UdpSockets() noexcept;

        /**
         * @brief Creates and configures UDP socket pair for DNS operations.
         *
         * @details Opens resolver socket for upstream communication and
         *          listener socket bound to specified port for client queries.
         *          Configures sockets for optimal DNS protocol handling.
         *
         * @param resolverAddress Resolved address of the upstream DNS server.
         * @param listenerPort Port number for client DNS query listener socket.
         *
         * @return Configured UdpSockets instance with valid socket descriptors.
         */
        static std::unique_ptr<UdpSockets> openUdpSockets(const sockaddr_storage &resolverAddress,
                                                          uint16_t listenerPort);

        /**
         * @brief Manually closes both managed UDP sockets.
         *
         * @details Explicitly closes resolver and listener sockets and
         *          invalidates file descriptors for safe resource cleanup.
         */
        void closeUdpSockets() noexcept;

        /**
         * @brief Returns resolver socket file descriptor.
         *
         * @details Provides access to resolver socket for upstream DNS
         *          server communication and response handling operations.
         *
         * @return File descriptor of resolver socket.
         */
        [[nodiscard]]
        int getResolverSocketFd() const noexcept;

        /**
         * @brief Returns listener socket file descriptor.
         *
         * @details Provides access to listener socket for client DNS
         *          query reception and request processing operations.
         *
         * @return File descriptor of listener socket.
         */
        [[nodiscard]]
        int getListenerSocketFd() const noexcept;

    private:
        /**
         * @brief Creates and configures a UDP socket for DNS communication.
         *
         * @details Low-level socket creation with proper options and
         *          configuration for efficient DNS protocol handling.
         *
         * @param family Address family for the socket (AF_INET or AF_INET6).
         *
         * @return File descriptor of created UDP socket.
         */
        static int createUdpSocket(int family);

        /**
         * @brief Connects resolver socket to specified DNS server address.
         *
         * @details Establishes connection for resolver socket to upstream
         *          DNS server using provided IP address and port number.
         *
         * @param resolverSocketFd File descriptor of the upstream DNS resolver socket
         * @param resolverAddress Resolved address of the upstream DNS resolver.
         */
        static void connectResolverSocket(int resolverSocketFd, const sockaddr_storage &resolverAddress);

        /**
         * @brief Closes resolver socket and invalidates descriptor.
         *
         * @details Internal method for resolver socket cleanup with
         *          proper error handling and descriptor invalidation.
         */
        void closeResolverSocket();

        /**
         * @brief Closes listener socket and invalidates descriptor.
         *
         * @details Internal method for listener socket cleanup with
         *          proper error handling and descriptor invalidation.
         */
        void closeListenerSocket();

        int mResolverSocketFd{INVALID_FD};  /**< File descriptor for upstream DNS resolver socket.     */
        int mListenerSocketFd{INVALID_FD};  /**< File descriptor for client DNS query listener socket. */
    }; // UdpSockets
} // FilteringDnsResolver::Networking

#endif // UDP_SOCKETS_HPP

/*** end of file UdpSockets.hpp ***/
