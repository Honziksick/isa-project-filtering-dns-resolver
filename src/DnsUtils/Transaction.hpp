/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         Transaction.hpp                                               *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      08.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides `Transaction` class, which          *
 *               represents a DNS transaction context for tracking client      *
 *               requests and managing transaction state in the DNS resolver.  *
 *               It stores client connection information, original transaction *
 *               IDs, and timestamps for timeout handling and response         *
 *               correlation in asynchronous DNS query processing.             *
 *                                                                             *
 ******************************************************************************/
/**
 * @file Transaction.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring `Transaction` class for DNS transaction
 *        context management and client request tracking.
 */

#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <netinet/in.h>  // sockaddr_in
#include <cstdint>       // uint16_t
#include <chrono>        // std::chrono

namespace FilteringDnsResolver::DnsUtils
{
    /**
     * @class Transaction
     * @brief Represents a DNS transaction context for tracking client requests.
     *
     * @details Stores essential information for correlating DNS queries with
     *          responses in asynchronous processing. Tracks client address,
     *          original transaction ID, and creation timestamp for timeout
     *          handling and proper response routing.
     */
    class Transaction {
    public:
        /**
         * @brief Default constructor creating an empty transaction.
         *
         * @details Initializes all fields to default values. The transaction
         *          must be populated with actual data before use.
         */
        Transaction() = default;

        /**
         * @brief Constructs a transaction with client information and original ID.
         *
         * @details Creates a transaction context with client address and original
         *          transaction ID. Timestamp is automatically set to current time
         *          for timeout tracking and transaction lifecycle management.
         *
         * @param clientAddress Client's network address for response routing.
         * @param originalId Original transaction ID from the client's query.
         */
        explicit Transaction(const sockaddr_in &clientAddress, uint16_t originalId);

        sockaddr_in mClientAddress{};                        /**< Client's network address for response routing.       */
        uint16_t mOriginalId{};                              /**< Original transaction ID from client's query.         */
        std::chrono::steady_clock::time_point mTimestamp{};  /**< Transaction creation timestamp for timeout handling. */
    }; // Transaction
} // FilteringDnsResolver::DnsUtils

#endif // TRANSACTION_HPP

/*** end of file Transaction.hpp ***/
