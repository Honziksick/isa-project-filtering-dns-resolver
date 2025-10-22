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
 *
 */

#ifndef UDP_FSM_HPP
#define UDP_FSM_HPP

#include "Networking/ClientJob.hpp"
#include "DnsUtils/DnsMessenger.hpp"
#include "DnsUtils/DnsForwarder.hpp"
#include "Filter/DomainFilter.hpp"
#include "Networking/UdpSockets.hpp"
#include "Utilities/TSQueue.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <cstdint>       // uint8_t, uint16_t
#include <memory>        // std::unique_ptr
#include <chrono>        // std::chrono
#include <poll.h>        // pollfd
#include <thread>        // std::thread
#include <atomic>        // std::atomic
#include <mutex>         // std::mutex

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
         * @brief Destructor for UDP FSM.
         */
        ~UdpFsm();

        /**
         * @brief Starts the main FSM event loop for DNS processing.
         *
         * @details Executes the primary event loop using poll-based I/O multiplexing
         *          to handle incoming DNS queries, upstream responses, and periodic
         *          maintenance tasks. Runs continuously until termination signal.
         */
        void run();

    private:
        // FDs
        std::unique_ptr<UdpSockets> mSocketFds;  /**< File descriptor for upstream DNS resolver socket and client listener socket. */

        // Dependencies
        std::unique_ptr<Filter::DomainFilter> mDomainFilterPtr;           /**< Domain filtering engine for query processing. */
        std::unique_ptr<DnsUtils::DnsForwarder> mForwarderPtr;            /**< DNS message forwarding component.             */
        std::unique_ptr<DnsUtils::DnsMessenger> mMessengerPtr;            /**< DNS message handling and parsing component.   */
        std::chrono::steady_clock::time_point mNextMaintenanceTimestamp;  /**< Timestamp for next maintenance cycle.         */
        Utilities::TSQueue<ClientJob> mClientQueue{Utilities::TSQueue<ClientJob>(CLIENT_QUEUE_CAPACITY)};  /**< Thread-safe queue for client jobs. */

        // Threading and concurrency
        mutable std::mutex mMutex{};                        /**< Mutex for protecting shared resources.                        */
        std::vector<std::thread> mWorkerThreads{};          /**< Container for worker thread objects.                          */
        std::thread mListenerThread{};                      /**< Thread for listening to incoming client DNS queries.          */
        std::thread mResolverThread{};                      /**< Thread for handling responses from upstream resolver.         */
        std::atomic<bool> mAreWorkerThreadsRunning{false};  /**< Indicates if worker threads are currently running.            */
        std::atomic<bool> mAreLisResThreadsRunning{false};  /**< Indicates if listener/resolver threads are currently running. */

        // Constants
        static constexpr auto POLL_TIMEOUT_MS{100};                                     /**< Poll timeout in milliseconds for event waiting.          */
        static constexpr auto MAINTENANCE_INTERVAL_MS{std::chrono::milliseconds{100}};  /**< Interval for periodic maintenance operations.            */
        static constexpr auto MAIN_LOOP_SLEEP_MS{std::chrono::milliseconds{10}};        /**< Sleep duration in main FSM loop to prevent busy-waiting. */
        static constexpr int POLL_FD_COUNT_IN_ONE_THREAD{1};                            /**< Number of file descriptors monitored in a single thread. */
        static constexpr int WORKER_THREAD_COUNT{4};                                    /**< Number of worker threads for parallel query processing.  */
        static constexpr auto LISTENER_READ_TIMEOUT_MS{std::chrono::milliseconds{1}};   /**< Time budget for reading listener events.                 */
        static constexpr auto RESOLVER_READ_TIMEOUT_MS{std::chrono::milliseconds{1}};   /**< Time budget for reading resolver events.                 */
        static constexpr size_t CLIENT_QUEUE_CAPACITY{8192};                            /**< Maximum capacity of the client job queue.                */
        static constexpr size_t CLIENT_QUEUE_THROTTLE_LIMIT{4096};                      /**< Throttle limit for client job queue size.                */

        /**
         * @brief Starts all worker threads for parallel DNS query processing.
         *
         * @details Initializes and launches worker threads that process DNS
         *          queries from clients in parallel, improving throughput and
         *          responsiveness of the DNS resolver.
         */
        void startWorkerThreads();

        /**
         * @brief Stops all worker threads and joins them.
         *
         * @details Signals all worker threads to terminate, waits for their
         *          completion, and ensures proper cleanup of resources
         *          associated with parallel query processing.
         */
        void stopWorkerThreads();

        /**
         * @brief Starts listener and resolver I/O threads.
         *
         * @details Launches dedicated threads for monitoring incoming client
         *          DNS queries and upstream resolver responses, enabling
         *          asynchronous network event handling and efficient
         *          I/O multiplexing.
         */
        void startLisResThreads();

        /**
         * @brief Stops listener and resolver I/O threads.
         *
         * @details Signals listener and resolver threads to terminate, waits
         *          for their completion, and ensures proper resource cleanup
         *          for network event processing.
         */
        void stopLisResThreads();

        /**
         * @brief Main loop executed by each worker thread.
         *
         * @details Continuously dequeues client DNS jobs from the thread-safe
         *          queue, applies domain filtering, and forwards queries or
         *          generates responses as needed. Each worker operates
         *          independently.
         *
         * @param workerIndex Index of the worker thread.
         */
        void workerLoop(size_t workerIndex);

        /**
         * @brief Main loop for the listener thread to process client DNS queries.
         *
         * @details Continuously monitors the listener socket for incoming
         *          DNS queries from clients, enqueues received jobs into the
         *          thread-safe queue for worker processing.
         */
        void listenerLoop();

        /**
         * @brief Main loop for the resolver thread to process upstream DNS responses.
         *
         * @details Monitors the resolver socket for incoming DNS responses
         *          from upstream servers, matches responses to client
         *          transactions, and forwards them appropriately.
         */
        void resolverLoop();

        /**
         * @brief Configures pollfd structure for listener socket event monitoring.
         *
         * @details Sets up the pollfd structure to monitor the listener
         *          socket for readable events, enabling efficient detection
         *          of incoming client DNS queries.
         *
         * @param fdWatcher Reference to pollfd structure to be configured.
         */
        void setupListenerPollFd(pollfd &fdWatcher) const;

        /**
         * @brief Configures pollfd structure for resolver socket event monitoring.
         *
         * @details Sets up the pollfd structure to monitor the resolver socket
         *          for readable events, enabling efficient detection of
         *          incoming DNS responses from upstream resolvers.
         *
         * @param fdWatcher Reference to pollfd structure to be configured.
         */
        void setupResolverPollFd(pollfd &fdWatcher) const;

        /**
         * @brief Waits for socket events using poll system call.
         *
         * @details Performs poll operation to wait for network events on monitored
         *          sockets with configured timeout for responsive event handling.
         *
         * @param fdWatcher Address of the configured pollfd variable.
         *
         * @return Number of file descriptors with pending events, or poll
         *         error code.
         */
        static int pollEvents(pollfd &fdWatcher);

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
