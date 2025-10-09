/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsForwarder.cpp                                              *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    08.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements `DnsForwarder` class, which       *
 *               handles forwarding of DNS queries to upstream resolvers and   *
 *               mapping responses back to original clients. It manages        *
 *               transaction ID mapping to prevent conflicts and tracks        *
 *               pending transactions with timeout handling.                   *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsForwarder.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DnsForwarder` class for DNS query
 *        forwarding and response mapping functionality.
 */

#include "DnsUtils/DnsForwarder.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "DnsUtils/Transaction.hpp"
#include "DnsUtils/TransactionIdProvider.hpp"
#include "Constants/CustomLimits.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/CastUtils.hpp"
#include "Utilities/Logger.hpp"
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // sendto()
#include <arpa/inet.h>  // inet_ntoa(), ntohs()
#include <cstring>      // strerror()
#include <cstdint>      // uint8_t, uint16_t
#include <vector>       // std::vector
#include <chrono>       // std::chrono
#include <cerrno>       // errno

using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::DnsUtils
{
    DnsForwarder::DnsForwarder(const int resolverFd)
        : mResolverFd{resolverFd} {
        logger("DnsForwarder constructor: initialized with resolver socket FD=%d", resolverFd);
        verbose("DNS forwarder initialized for upstream communication");
    } // DnsForwarder::DnsForwarder

    void DnsForwarder::forwardQueryToResolver(const uint8_t *messageBuffer,
                                              const size_t messageLength,
                                              const sockaddr_in &clientAddress) {
        logger("DnsForwarder::forwardQueryToResolver() called with messageLength=%zu bytes, client=%s:%u",
               messageLength, inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // First we validate the message length
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Query validation failed: message too short (%zu bytes < %zu required)",
                   messageLength, DnsQuery::HEADER_TRUE_SIZE);
            verbose("Dropping malformed DNS query (too short)");
            return;
        }
        if(messageLength > CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE) {
            logger("Query validation failed: message too long (%zu bytes > %zu limit)",
                   messageLength, CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE);
            verbose("Dropping oversized DNS query");
            return;
        }

        logger("Message length validation passed: %zu bytes", messageLength);

        // Then we create a modifiable copy of the message and extract the original ID
        logger("Creating modifiable copy of DNS message");
        vector<uint8_t> updatedMessage = CastUtils::castByteArrayToVector(messageBuffer, messageLength);
        const uint16_t originalId = CastUtils::castTwoBytesToWord(updatedMessage, 0);
        logger("Extracted original DNS transaction ID: %u (0x%04X)", originalId, originalId);

        // We use the TransactionIdProvider to get a new unique ID
        logger("Requesting new transaction ID from provider");
        const uint16_t newId = mIdProvider.getNextId();

        if(newId == TransactionIdProvider::ALL_IDS_USED) {
            logger("Transaction ID allocation failed: all IDs exhausted for originalId=%u", originalId);
            verbose("DNS query dropped - transaction limit reached");
            return;
        }

        logger("Allocated new transaction ID: %u (0x%04X) for original ID: %u", newId, newId, originalId);

        // Update the message with the new ID
        logger("Updating DNS message header with new transaction ID");
        const vector<uint8_t> newIdAsBytes = CastUtils::castWordToTwoBytes<uint16_t>(newId);
        updatedMessage[0] = newIdAsBytes[0];
        updatedMessage[1] = newIdAsBytes[1];
        logger("DNS message header updated: ID field set to %u", newId);

        // Save the mapping of new ID to original ID and client address
        logger("Creating transaction mapping: newId=%u -> originalId=%u, client=%s:%u",
               newId, originalId, inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        mPendingTransactions[newId] = Transaction{clientAddress, originalId};
        logger("Transaction stored in pending map, total pending transactions: %zu", mPendingTransactions.size());

        // Finally, we send the modified message to the resolver
        logger("Sending DNS query to upstream resolver via socket FD=%d, size=%zu bytes",
               mResolverFd, updatedMessage.size());
        const ssize_t bytesSent = send(mResolverFd, updatedMessage.data(), updatedMessage.size(), 0);

        // If the sendto() function returns an error
        if(bytesSent < 0) {
            logger("send() failed: socket FD=%d, error=%s, errno=%d, attempted bytes=%zu",
                   mResolverFd, strerror(errno), errno, updatedMessage.size());
            verbose("Failed to forward DNS query to upstream resolver");

            // Clean up the transaction since forwarding failed
            logger("Cleaning up failed transaction: releasing ID=%u and removing mapping", newId);
            mIdProvider.releaseId(newId);
            mPendingTransactions.erase(newId);

            throw ConnectionErrorException(
                    "Failed to forward DNS query to the resolver due to sendto() error: " + string(strerror(errno))
                    );
        }
        // If the sendto() function returns 0, it means the connection has been closed
        if(bytesSent == 0) {
            logger("send() returned 0: connection closed by upstream resolver, socket FD=%d", mResolverFd);
            verbose("Connection to upstream resolver was closed");

            // Clean up the transaction since connection is closed
            logger("Cleaning up transaction due to closed connection: releasing ID=%u", newId);
            mIdProvider.releaseId(newId);
            mPendingTransactions.erase(newId);

            throw ConnectionErrorException(
                    "Connection closed by resolver. Unable to forward DNS query to the resolver."
                    );
        }

        logger("DNS query forwarded successfully: originalId=%u -> newId=%u, bytes sent=%zd/%zu",
               originalId, newId, bytesSent, updatedMessage.size());
        verbose("DNS query forwarded to upstream resolver");
    } // DnsForwarder::forwardQueryToResolver

    bool DnsForwarder::mapResponseFromResolver(uint8_t *messageBuffer,
                                               const size_t messageLength,
                                               sockaddr_in &clientAddress) {
        logger("DnsForwarder::mapResponseFromResolver() called with messageLength=%zu bytes", messageLength);

        // First we validate the response length
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Response validation failed: message too short (%zu bytes < %zu required)",
                   messageLength, DnsQuery::HEADER_TRUE_SIZE);
            verbose("Dropping malformed DNS response (too short)");
            return false;
        }
        if(messageLength > CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE) {
            logger("Response validation failed: message too long (%zu bytes > %zu limit)",
                   messageLength, CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE);
            verbose("Dropping oversized DNS response");
            return false;
        }

        logger("Response length validation passed: %zu bytes", messageLength);

        // Then we create a copy of the message and extract the original ID
        logger("Extracting transaction ID from DNS response");
        const vector<uint8_t> responseVector = CastUtils::castByteArrayToVector(messageBuffer, messageLength);
        const uint16_t responseId = CastUtils::castTwoBytesToWord(responseVector, 0);
        logger("Extracted response transaction ID: %u (0x%04X)", responseId, responseId);


        // Look up the mapping
        logger("Looking up transaction mapping for ID=%u in %zu pending transactions",
               responseId, mPendingTransactions.size());
        const auto transactionIt = mPendingTransactions.find(responseId);
        if(transactionIt == mPendingTransactions.end()) {
            logger("Transaction lookup failed: unknown response ID=%u, no matching pending transaction", responseId);
            verbose("Received DNS response for unknown transaction");
            return false;
        }

        logger("Transaction mapping found: responseId=%u -> originalId=%u, client=%s:%u",
               responseId, transactionIt->second.mOriginalId,
               inet_ntoa(transactionIt->second.mClientAddress.sin_addr),
               ntohs(transactionIt->second.mClientAddress.sin_port));

        // Set original client ID back (network byte order, big-endian)
        const uint16_t originalId = transactionIt->second.mOriginalId;
        logger("Restoring original transaction ID in response header: %u -> %u", responseId, originalId);

        const vector<uint8_t> originalIdBytes = CastUtils::castWordToTwoBytes<uint16_t>(originalId);
        messageBuffer[0] = originalIdBytes[0];
        messageBuffer[1] = originalIdBytes[1];
        logger("DNS response header updated with original ID: %u", originalId);

        // Return the packet back to the original client
        clientAddress = transactionIt->second.mClientAddress;
        logger("Client address set for response delivery: %s:%u",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // Release the used transaction ID and remove mapping
        logger("Cleaning up completed transaction: releasing ID=%u and removing mapping", responseId);
        mIdProvider.releaseId(responseId);
        mPendingTransactions.erase(transactionIt);
        logger("Transaction cleanup completed, remaining pending transactions: %zu", mPendingTransactions.size());

        verbose("DNS response mapped back to original client");
        return true;
    } // DnsForwarder::mapResponseFromResolver

    void DnsForwarder::deleteOldTransactions() {
        const auto now = chrono::steady_clock::now();
        size_t expiredCount = 0;

        // Iterate through the map and remove old transactions
        auto transactionIt = mPendingTransactions.begin();
        while(transactionIt != mPendingTransactions.end()) {
            const auto transactionAge = now - transactionIt->second.mTimestamp;
            const auto ageMs = chrono::duration_cast<chrono::milliseconds>(transactionAge).count();

            if(transactionAge > PENDING_TXS_MAX_WAIT) {
                logger("Expiring transaction: ID=%u, age=%ldms (limit=%ldms), client=%s:%u",
                       transactionIt->first, ageMs,
                       chrono::duration_cast<chrono::milliseconds>(PENDING_TXS_MAX_WAIT).count(),
                       inet_ntoa(transactionIt->second.mClientAddress.sin_addr),
                       ntohs(transactionIt->second.mClientAddress.sin_port));

                mIdProvider.releaseId(transactionIt->first);
                transactionIt = mPendingTransactions.erase(transactionIt); // erase returns the next iterator
                expiredCount++;
            }
            else {
                transactionIt++;  // move to the next transaction
            }
        } // while

        if(expiredCount > 0) {
            logger("Transaction cleanup completed: expired %zu transactions, remaining %zu pending",
                   expiredCount, mPendingTransactions.size());
            verbose("Cleaned up %zu expired DNS transactions", expiredCount);
        }
        else {
            logger("Transaction cleanup completed: no expired transactions found, %zu still pending",
                   mPendingTransactions.size());
        }
    } // DnsForwarder::deleteOldTransactions
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsForwarder.cpp ***/
