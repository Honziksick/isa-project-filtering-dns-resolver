/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsForwarder.hpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `DnsForwarder` class, which         *
 *               handles forwarding of DNS queries to upstream resolvers and   *
 *               mapping responses back to original clients. It manages        *
 *               transaction ID mapping to prevent conflicts and tracks        *
 *               pending transactions with timeout handling.                   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsForwarder.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `DnsForwarder` class for DNS query forwarding
 *        and response mapping functionality.
 */

#ifndef DNS_FORWARDER_HPP
#define DNS_FORWARDER_HPP

#include "DnsUtils/Transaction.hpp"
#include "DnsUtils/TransactionIdProvider.hpp"
#include <unordered_map>  // std::unordered_map
#include <netinet/in.h>   // sockaddr_in
#include <cstdint>        // uint8_t
#include <chrono>         // std::chrono

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class DnsForwarder
     * @brief Handles DNS query forwarding and response mapping between clients
     *        and upstream resolvers.
     *
     * @details This class manages the forwarding of DNS queries from clients
     *          to upstream resolvers and the subsequent mapping of responses
     *          back to the original clients. It maintains transaction ID mapping
     *          to prevent conflicts when multiple clients use the same transaction
     *          IDs, and provides timeout handling for pending transactions to
     *          prevent resource leaks.
     *
     * @note This class uses a connection-oriented socket to communicate with
     *       the upstream resolver, requiring the socket to be properly
     *       configured before use.
     */
    class DnsForwarder final {
    public:
        /**
         * @brief Constructs a DnsForwarder with the specified resolver socket.
         *
         * @details Initializes the forwarder with a socket file descriptor
         *          that should be connected to an upstream DNS resolver. The
         *          socket must remain valid for the lifetime of this object.
         *
         * @param resolverFd File descriptor of the socket connected to the
         *                   upstream resolver.
         *
         * @warning The caller is responsible for ensuring the socket remains
         *          valid and properly configured for the lifetime of this object.
         */
        explicit DnsForwarder(int resolverFd);

        /**
         * @brief Forwards a DNS query to the upstream resolver with transaction
         *        ID mapping.
         *
         * @details This method takes a DNS query from a client, replaces its
         *          transaction ID with a unique ID from the ID provider, stores
         *          the mapping between the new and original IDs along with the
         *          client address, and forwards the modified query to the
         *          upstream resolver.
         *
         * @param messageBuffer Pointer to the DNS query message buffer.
         * @param messageLength Length of the DNS query message in bytes.
         * @param clientAddress Address of the client that sent the original query.
         *
         * @throws ConnectionErrorException If sending to the upstream resolver fails.
         *
         * @note If no free transaction IDs are available, the query is dropped and logged.
         */
        void forwardQueryToResolver(const uint8_t *messageBuffer, size_t messageLength, const sockaddr_in &clientAddress);

        /**
         * @brief Maps a DNS response from the upstream resolver back to the
         *        original client.
         *
         * @details This method takes a DNS response from the upstream resolver,
         *          looks up the corresponding transaction mapping, restores the
         *          original transaction ID, retrieves the original client address,
         *          and cleans up the transaction state. The modified response
         *          can then be sent back to the client.
         *
         * @param messageBuffer Pointer to the DNS response message buffer (modified in-place).
         * @param messageLength Length of the DNS response message in bytes.
         * @param clientAddress Reference to store the original client address.
         *
         * @return `true` if the response was successfully mapped to a pending transaction,
         *         `false` if no matching transaction was found or message validation failed.
         *
         * @note The original transaction ID is restored in the message buffer in network byte order.
         * @note Successful mapping automatically releases the transaction ID and removes the mapping.
         */
        bool mapResponseFromResolver(uint8_t *messageBuffer, size_t messageLength, sockaddr_in &clientAddress);

        /**
         * @brief Removes expired transactions that have exceeded the maximum wait time.
         *
         * @details This maintenance method iterates through all pending transactions
         *          and removes those that have been waiting longer than the configured
         *          timeout. It releases the associated transaction IDs back to
         *          the ID provider and logs the expired transactions for debugging
         *          purposes.
         *
         * @note This method should be called periodically to prevent resource
         *       leaks from transactions that never receive responses from the
         *       upstream resolver.
         * @note The timeout period is defined by the `PENDING_TXS_MAX_WAIT` constant.
         */
        void deleteOldTransactions();

    private:
        int mResolverFd;                                                   /**< File descriptor of the upstream resolver socket */
        TransactionIdProvider mIdProvider{};                               /**< Provider for unique transaction IDs             */
        std::unordered_map<uint16_t, Transaction> mPendingTransactions{};  /**< Map of pending transactions by resolver ID      */

        /**
         * @brief Maximum time to wait for a response before considering a
         *        transaction expired.
         *
         * @details Transactions that exceed this timeout are automatically
         *          removed by the `deleteOldTransactions()` method to prevent
         *          resource leaks.
         */
        static constexpr auto PENDING_TXS_MAX_WAIT{std::chrono::milliseconds(5000)};
    }; // DnsForwarder
} // FilteringDnsResolver::DnsUtils

#endif // DNS_FORWARDER_HPP

/*** end of file DnsForwarder.hpp ***/
