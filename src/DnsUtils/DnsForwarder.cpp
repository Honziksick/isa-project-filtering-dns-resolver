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
#include <sys/socket.h> // sendto()
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
        : mResolverFd{resolverFd} {}

    void DnsForwarder::forwardQueryToResolver(const uint8_t *messageBuffer,
                                              const size_t messageLength,
                                              const sockaddr_in &clientAddress) {
        // First we validate the message length
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Query (to be forwarded) is too short (%zu B)", messageLength);
            return;
        }
        if(messageLength > CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE) {
            logger("Query (to be forwarded) is too long (%zu B)", messageLength);
            return;
        }

        // Then we create a modifiable copy of the message and extract the original ID
        vector<uint8_t> updatedMessage = CastUtils::castByteArrayToVector(messageBuffer, messageLength);
        const uint16_t originalId = CastUtils::castTwoBytesToWord(updatedMessage, 0);

        // We use the TransactionIdProvider to get a new unique ID
        const uint16_t newId = mIdProvider.getNextId();

        if(newId == TransactionIdProvider::ALL_IDS_USED) {
            logger("No free IDs for forwarding DNS transaction of 'originalId = %u'", originalId);
            return;
        }

        // Update the message with the new ID
        const vector<uint8_t> newIdAsBytes = CastUtils::castWordToTwoBytes<uint16_t>(newId);
        updatedMessage[0] = newIdAsBytes[0];
        updatedMessage[1] = newIdAsBytes[1];

        // Save the mapping of new ID to original ID and client address
        mPendingTransactions[newId] = Transaction{clientAddress, originalId};

        // Finally, we send the modified message to the resolver
        const ssize_t bytesSent = send(mResolverFd, updatedMessage.data(), updatedMessage.size(), 0);

        // If the sendto() function returns an error
        if(bytesSent < 0) {
            logger("sendto() returned error: Failed to send message to resolver. Socket 'FD = %d', "
                   "error: %s, Bytes: %zd", mResolverFd, strerror(errno), bytesSent);
            throw ConnectionErrorException(
                    "Failed to forward DNS query to the resolver due to sendto() error: " + string(strerror(errno))
                    );
        }
        // If the sendto() function returns 0, it means the connection has been closed
        if(bytesSent == 0) {
            logger("sendto() returned 0: Connection closed by resolver");
            throw ConnectionErrorException(
                    "Connection closed by resolver. Unable to forward DNS query to the resolver."
                    );
        }

        logger("DNS Query forwarded: 'originalId = %u' -> 'newId = %u', 'size = %zd'", originalId, newId, bytesSent);
    } // DnsForwarder::forwardQueryToResolver

    bool DnsForwarder::mapResponseFromResolver(uint8_t *messageBuffer,
                                               const size_t messageLength,
                                               sockaddr_in &clientAddress) {
        // First we validate the response length
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Response (to be forwarded) is too short (%zu B)", messageLength);
            return false;
        }
        if(messageLength > CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE) {
            logger("Response (to be forwarded) is too long (%zu B)", messageLength);
            return false;
        }

        // Then we create a copy of the message and extract the original ID
        const vector<uint8_t> responseVector = CastUtils::castByteArrayToVector(messageBuffer, messageLength);
        const uint16_t responseId = CastUtils::castTwoBytesToWord(responseVector, 0);

        // Look up the mapping
        const auto transactionIt = mPendingTransactions.find(responseId);
        if(transactionIt == mPendingTransactions.end()) {
            logger("DnsForwarder::mapResponse(): unknown id=%u", responseId);
            return false;
        }

        // Set original client ID back (network byte order, big-endian)
        const uint16_t originalId = transactionIt->second.mOriginalId;
        const vector<uint8_t> originalIdBytes = CastUtils::castWordToTwoBytes<uint16_t>(originalId);
        messageBuffer[0] = originalIdBytes[0];
        messageBuffer[1] = originalIdBytes[1];

        // Return the packet back to the original client
        clientAddress = transactionIt->second.mClientAddress;

        // Release the used transaction ID and remove mapping
        mIdProvider.releaseId(responseId);
        mPendingTransactions.erase(transactionIt);

        return true;
    } // DnsForwarder::mapResponseFromResolver

    void DnsForwarder::deleteOldTransactions() {
        const auto now = chrono::steady_clock::now();

        // Iterate through the map and remove old transactions
        auto transactionIt = mPendingTransactions.begin();
        while(transactionIt != mPendingTransactions.end()) {
            if(now - transactionIt->second.mTimestamp > PENDING_TXS_MAX_WAIT) {
                logger("DnsForwarder::purge(): expiring id=%u", transactionIt->first);

                mIdProvider.releaseId(transactionIt->first);
                transactionIt = mPendingTransactions.erase(transactionIt); // erase returns the next iterator
            }
            else {
                transactionIt++;  // Move to the next transaction
            }
        } // while
    } // DnsForwarder::deleteOldTransactions
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsForwarder.cpp ***/
