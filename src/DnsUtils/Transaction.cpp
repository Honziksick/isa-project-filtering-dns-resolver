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
#include <netinet/in.h>  // sockaddr_in
#include <chrono>        // std::chrono::steady_clock
#include <cstdint>       // uint16_t

namespace FilteringDnsResolver::DnsUtils
{
    Transaction::Transaction(const sockaddr_in &clientAddress, const uint16_t originalId)
        : mClientAddress{clientAddress},
          mOriginalId{originalId},
          mTimestamp{std::chrono::steady_clock::now()} {}
} // FilteringDnsResolver::DnsUtils

/*** end of file Transaction.cpp ***/
