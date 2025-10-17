/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         UdpFsm.hpp                                                    *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    09.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `UdpFsm` class, which implements    *
 *               the UDP finite state machine for DNS protocol handling in     *
 *               the filtering DNS resolver. It manages asynchronous UDP       *
 *               communication using poll-based event handling to process      *
 *               incoming DNS queries from clients and responses from upstream *
 *               resolvers. The FSM coordinates domain filtering, message      *
 *               forwarding, and transaction management for efficient DNS      *
 *               proxy operation with real-time performance characteristics.   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file UdpFsm.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `UdpFsm` class for UDP finite state machine
 *        DNS protocol handling and asynchronous network event processing.
 */

#ifndef UDP_FSM_HPP
#define UDP_FSM_HPP

#include "DnsUtils/DnsMessenger.hpp"
#include "DnsUtils/DnsForwarder.hpp"
#include "Filter/DomainFilter.hpp"
#include "Networking/UdpSockets.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <cstdint>       // uint8_t, uint16_t
#include <memory>        // std::unique_ptr
#include <chrono>        // std::chrono
#include <poll.h>        // pollfd

namespace FilteringDnsResolver::Networking
{
    /**
     * @class UdpFsm
     * @brief UDP finite state machine for DNS protocol handling and event processing.
     *
     * @details Implements asynchronous UDP communication using poll-based event
     *          handling to manage DNS queries and responses. Coordinates domain
     *          filtering, message forwarding, and transaction lifecycle management
     *          for efficient DNS proxy operation with real-time performance.
     */
    class UdpFsm final {
    public:
        /**
         * @brief Constructs UDP FSM with socket descriptors and domain filter.
         *
         * @details Initializes the finite state machine with resolver and listener
         *          sockets, domain filtering capability, and creates DNS forwarding
         *          and messaging components for complete DNS protocol handling.
         *
         * @param socketFds File descriptors for upstream DNS resolver communication
         *                  and client DNS query listener socket.
         * @param domainFilter Unique pointer to domain filtering engine.
         */
        explicit UdpFsm(std::unique_ptr<UdpSockets> socketFds,
                        std::unique_ptr<Filter::DomainFilter> domainFilter);

        /**
         * @brief Starts the main FSM event loop for DNS processing.
         *
         * @details Executes the primary event loop using poll-based I/O multiplexing
         *          to handle incoming DNS queries, upstream responses, and periodic
         *          maintenance tasks. Runs continuously until termination signal.
         */
        void run();

    private:
        // Constants
        static constexpr auto POLL_TIMEOUT_MS{1000};                                  /**< Poll timeout in milliseconds for event waiting. */
        static constexpr auto MAINTENANCE_INTERVAL{std::chrono::milliseconds(1000)};  /**< Interval for periodic maintenance operations.   */

        static constexpr auto MAX_RESOLVER_PACKETS_PER_ITERATION{100};  /**< Max resolver packets to process per loop iteration. */
        static constexpr int MAX_CLIENT_PACKETS_PER_ITERATION{100};     /**< Max client packets to process per loop iteration.   */

        static constexpr int POLL_FD_COUNT{2};        /**< Number of file descriptors monitored by poll. */
        static constexpr int POLL_RESOLVER_INDEX{0};  /**< Poll array index for resolver socket. */
        static constexpr int POLL_LISTENER_INDEX{1};  /**< Poll array index for listener socket. */

        // FDs
        std::unique_ptr<UdpSockets> mSocketFds;  /**< File descriptor for upstream DNS resolver socket and client listener socket. */

        // Dependencies
        std::unique_ptr<Filter::DomainFilter> mDomainFilterPtr;           /**< Domain filtering engine for query processing. */
        std::unique_ptr<DnsUtils::DnsForwarder> mForwarderPtr;            /**< DNS message forwarding component.             */
        std::unique_ptr<DnsUtils::DnsMessenger> mMessengerPtr;            /**< DNS message handling and parsing component.   */
        std::chrono::steady_clock::time_point mNextMaintenanceTimestamp;  /**< Timestamp for next maintenance cycle.         */

        /**
         * @brief Configures poll file descriptor structure for event monitoring.
         *
         * @details Sets up pollfd structure with file descriptors and event masks
         *          for monitoring resolver and listener sockets during poll operations.
         *
         * @param fdWatcher Pointer to pollfd array for configuration.
         */
        void setupPollFd(pollfd *fdWatcher) const;

        /**
         * @brief Waits for socket events using poll system call.
         *
         * @details Performs poll operation to wait for network events on monitored
         *          sockets with configured timeout for responsive event handling.
         *
         * @param fdWatcher Pointer to configured pollfd array.
         *
         * @return Number of file descriptors with pending events, or poll
         *         error code.
         */
        static int pollEvents(pollfd *fdWatcher);

        /**
         * @brief Handles incoming DNS query from client.
         *
         * @details Processes DNS query datagram from client, applies domain filtering,
         *          and either forwards to upstream resolver or generates filtered response
         *          based on domain filter configuration and query content.
         *
         * @param messageBuffer Pointer to received DNS message data.
         * @param messageLength Length of received DNS message in bytes.
         * @param clientAddress Socket address structure of requesting client.
         */
        void onClientDatagram(const uint8_t *messageBuffer, size_t messageLength,
                              const sockaddr_in &clientAddress) const;

        /**
         * @brief Handles DNS response from upstream resolver.
         *
         * @details Processes DNS response datagram from upstream resolver and
         *          forwards the response back to the appropriate client based
         *          on transaction tracking and message correlation.
         *
         * @param messageBuffer Pointer to received DNS response data.
         * @param messageLength Length of received DNS response in bytes.
         */
        void onResolverDatagram(uint8_t *messageBuffer, size_t messageLength) const;

        /**
         * @brief Performs periodic transaction cleanup and maintenance.
         *
         * @details Executes periodic maintenance tasks including transaction
         *          timeout handling, memory cleanup, and state consistency
         *          checks to ensure optimal FSM operation and resource
         *          management.
         */
        void transactionsMaintenance();
    }; // UdpFsm
} // FilteringDnsResolver::Networking

#endif // UDP_FSM_HPP

/*** end of file UdpFsm.hpp ***/
