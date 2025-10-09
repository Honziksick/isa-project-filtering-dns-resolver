/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         Transaction.cpp                                               *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      08.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `Transaction` class, which    *
 *               represents a DNS transaction context for tracking client      *
 *               requests and managing transaction state in the DNS resolver.  *
 *               It stores client connection information, original transaction *
 *               IDs, and timestamps for timeout handling and response         *
 *               correlation in asynchronous DNS query processing.             *
 *                                                                             *
 ******************************************************************************/
/**
 * @file Transaction.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `Transaction` class for DNS transaction
 *        context management and client request tracking.
 */

#include "DnsUtils/Transaction.hpp"
#include "Utilities/Logger.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <arpa/inet.h>   // inet_ntoa(), ntohs()
#include <chrono>        // std::chrono::steady_clock
#include <cstdint>       // uint16_t

namespace FilteringDnsResolver::DnsUtils
{
    Transaction::Transaction(const sockaddr_in &clientAddress, const uint16_t originalId)
        : mClientAddress{clientAddress},
          mOriginalId{originalId},
          mTimestamp{std::chrono::steady_clock::now()} {
        logger("Transaction::Transaction() constructor called with parameters:");
        logger("  Client address: %s:%u", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        logger("  Original transaction ID: %u (0x%04X)", originalId, originalId);
        logger("  Timestamp: %lld nanoseconds since epoch", static_cast<long long>(mTimestamp.time_since_epoch().count()));

        verbose("Created DNS transaction for client %s:%u (ID: %u)",
                inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), originalId);

        logger("Transaction object successfully constructed");
    }
} // FilteringDnsResolver::DnsUtils

/*** end of file Transaction.cpp ***/
