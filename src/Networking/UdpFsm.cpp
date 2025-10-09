/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         UdpFsm.cpp                                                    *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      07.10.2025                                                    *
 * Last edit:    09.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `UdpFsm` class, which is      *
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
 * @file UdpFsm.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `UdpFsm` class for UDP finite state
 *        machine DNS protocol handling and asynchronous network event
 *        processing.
 */

#include "Networking/UdpFsm.hpp"
#include "DnsUtils/DnsMessageParser.hpp"
#include "DnsUtils/DnsMessenger.hpp"
#include "DnsUtils/DnsForwarder.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "Constants/CustomLimits.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include "Utilities/CastUtils.hpp"
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // recvfrom(), recv()
#include <arpa/inet.h>   // inet_ntoa(), ntohs()
#include <cstdint>       // uint8_t, uint16_t
#include <cstring>       // strerror()
#include <cerrno>        // errno
#include <poll.h>        // pollfd, poll()
#include <utility>       // std::move
#include <memory>        // std::unique_ptr, std::make_unique
#include <chrono>        // std::chrono
#include <array>         // std::array

using namespace FilteringDnsResolver::DnsUtils;
using namespace FilteringDnsResolver::Filter;
using namespace FilteringDnsResolver::Constants;
using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Exceptions;
using namespace FilteringDnsResolver::Utilities;
using namespace std;

namespace FilteringDnsResolver::Networking
{
    UdpFsm::UdpFsm(unique_ptr<UdpSockets> socketFds, unique_ptr<DomainFilter> domainFilter)
        : mSocketFds{move(socketFds)},
          mDomainFilterPtr{move(domainFilter)},
          mForwarderPtr{make_unique<DnsForwarder>(mSocketFds->getResolverSocketFd())},
          mMessengerPtr{make_unique<DnsMessenger>(mSocketFds->getListenerSocketFd())},
          mNextMaintenanceTimestamp{chrono::steady_clock::now() + MAINTENANCE_INTERVAL} {
        logger("UdpFsm constructor: initializing with listenerFd=%d, resolverFd=%d",
               mSocketFds->getListenerSocketFd(), mSocketFds->getResolverSocketFd());

        if(mSocketFds->getListenerSocketFd() < 0) {
            logger("ERROR: Invalid listener socket FD: %d", mSocketFds->getListenerSocketFd());
            throw InternalErrorException("Invalid listen socket FD");
        }
        if(mSocketFds->getResolverSocketFd() < 0) {
            logger("ERROR: Invalid resolver socket FD: %d", mSocketFds->getResolverSocketFd());
            throw InternalErrorException("Invalid resolver socket FD");
        }
        if(!mDomainFilterPtr) {
            logger("ERROR: Domain filter pointer is null");
            throw InternalErrorException("Domain filter pointer is null");
        }

        logger("UdpFsm initialized successfully with maintenance interval");
        verbose("DNS filtering resolver started and ready to accept queries");
    } // UdpFsm::UdpFsm

    void UdpFsm::run() {
        logger("UdpFsm::run(): starting main event loop");
        verbose("DNS resolver is now listening for queries...");

        array<uint8_t, CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE> buffer{};
        logger("Allocated receive buffer of %zu bytes", buffer.size());

        while(true) {
            pollfd fdWatcher[POLL_FD_COUNT]{};
            setupPollFd(fdWatcher);

            const int eventCount = pollEvents(fdWatcher);

            if(eventCount > 0) {
                logger("Poll returned %d active events", eventCount);

                // Check if a new datagram has arrived from a client
                if(fdWatcher[POLL_LISTENER_INDEX].revents & POLLIN) {
                    logger("Client query received on listener socket");

                    sockaddr_in clientAddress{};
                    socklen_t clientAddressLength{sizeof(clientAddress)};
                    const ssize_t bytesReceived = recvfrom(mSocketFds->getListenerSocketFd(),
                                                           buffer.data(), buffer.size(), 0,
                                                           reinterpret_cast<sockaddr*>(&clientAddress),
                                                           &clientAddressLength); // we act as UDP server

                    // If the recvfrom() function returns an error
                    if(bytesReceived < 0) {
                        logger("ERROR: recvfrom() failed with errno=%d: %s", errno, strerror(errno));

                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            logger("recvfrom() returned EAGAIN/EWOULDBLOCK: datagram too large");
                            throw ProtocolErrorException("UDP datagram exceeds maximum size.");
                        }
                        else {
                            throw ConnectionErrorException(
                                    "Failed to receive data from the server due "
                                    "to recvfrom() error: " + string(strerror(errno))
                                    );
                        }
                    }
                    // If the recvfrom() function returns 0, it means the connection has been closed
                    if(bytesReceived == 0) {
                        logger("recvfrom() returned 0: connection closed");
                        throw ConnectionErrorException("Connection closed by server. No data received.");
                    }

                    logger("Received %zd bytes from client %s:%d", bytesReceived,
                           inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

                    // We process the received datagram
                    try {
                        onClientDatagram(buffer.data(), static_cast<size_t>(bytesReceived), clientAddress);
                    }
                    catch(const BaseCustomException<ExitCodes> &e) {
                        logger("onClientDatagram() exception: %s", e.what());
                        try {
                            mMessengerPtr->sendServFailMessage(buffer.data(), static_cast<size_t>(bytesReceived), clientAddress);
                        }
                        catch(...) {
                            // TODO: what behavior here?
                            logger("Failed to send SERVFAIL response to client");
                        }
                    }
                    catch(...) {
                        logger("onClientDatagram() unknown exception");
                        try {
                            mMessengerPtr->sendServFailMessage(buffer.data(), static_cast<size_t>(bytesReceived), clientAddress);
                        }
                        catch(...) {
                            // TODO: what behavior here?
                            logger("Failed to send SERVFAIL response to client");
                        }
                    }
                } // if(listener POLLIN)

                // Check if answer has arrived from the resolver
                if(fdWatcher[POLL_RESOLVER_INDEX].revents & POLLIN) {
                    logger("Response received from upstream resolver");

                    const ssize_t bytesReceived = recv(mSocketFds->getResolverSocketFd(),
                                                       buffer.data(), buffer.size(), 0); // we act as UDP client

                    // If the recv() function returns an error
                    if(bytesReceived < 0) {
                        logger("ERROR: recv() from resolver failed with errno=%d: %s", errno, strerror(errno));
                        throw ConnectionErrorException(
                                "Failed to receive data from the server due "
                                "to recv() error: " + string(strerror(errno))
                                );
                    }
                    // If the recv() function returns 0, it means the connection has been closed
                    if(bytesReceived == 0) {
                        logger("recv() returned 0: resolver connection closed");
                        throw ConnectionErrorException("Connection closed by server. No data received.");
                    }

                    logger("Received %zd bytes from upstream resolver", bytesReceived);

                    try {
                        onResolverDatagram(buffer.data(), static_cast<size_t>(bytesReceived));
                    }
                    catch(const BaseCustomException<ExitCodes> &e) {
                        logger("onResolverDatagram() exception: %s", e.what());
                    }
                    catch(...) {
                        logger("onResolverDatagram() unknown exception");
                        // TODO: what behavior here?
                    }
                } // if(resolver POLLIN)
            } // if(pollEvents() > 0)

            // Lastly, we perform maintenance tasks
            transactionsMaintenance();
        } // while(true)
    } // UdpFsm::run

    void UdpFsm::setupPollFd(pollfd *fdWatcher) const {
        // Listener socket
        fdWatcher[POLL_LISTENER_INDEX].fd = mSocketFds->getListenerSocketFd();
        fdWatcher[POLL_LISTENER_INDEX].events = POLLIN;

        // Resolver socket
        fdWatcher[POLL_RESOLVER_INDEX].fd = mSocketFds->getResolverSocketFd();
        fdWatcher[POLL_RESOLVER_INDEX].events = POLLIN;
    } // UdpFsm::setupPollFd

    int UdpFsm::pollEvents(pollfd *fdWatcher) {
        const int eventCount = poll(fdWatcher, POLL_FD_COUNT, POLL_TIMEOUT_MS);
        if(eventCount < 0) {
            if(errno == EINTR) {
                logger("poll() interrupted by signal (errno=EINTR)");
                verbose("DNS resolver interrupted by system signal");
                throw UserInterruptionException(
                        "poll() interrupted by signal: " + string(strerror(errno))
                        );
            }
            else {
                logger("ERROR: poll() failed with errno=%d: %s", errno, strerror(errno));
                throw ConnectionErrorException(
                        "poll() error: " + string(strerror(errno))
                        );
            }
        }
        return eventCount;
    } // UdpFsm::pollEvents

    void UdpFsm::onClientDatagram(const uint8_t *messageBuffer,
                                  const size_t messageLength,
                                  const sockaddr_in &clientAddress) const {
        logger("Processing client datagram from %s:%d (%zu bytes)",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port),
               messageLength);

        // Check minimum length
        if(messageLength < DnsQuery::HEADER_TRUE_SIZE) {
            logger("Client datagram too short: %zu bytes (minimum: %zu)",
                   messageLength, static_cast<size_t>(DnsQuery::HEADER_TRUE_SIZE));
            mMessengerPtr->sendFormErrMessage(messageBuffer, messageLength, clientAddress);
            return;
        }

        // We parse and validate the DNS query
        DnsQuery dnsQuery{};
        try {
            dnsQuery = DnsMessageParser::parseAndValidate(messageBuffer, messageLength);
            logger("DNS query parsed successfully: domain='%s'", dnsQuery.mQName.c_str());
        }
        catch(const DnsParseErrorException &e) {
            logger("DNS parse error (code=%d): %s", e.code(), e.what());

            switch(e.code()) {
                // The query is malformed
                case CastUtils::castEnumToInt(DnsRCodes::FORMERR):
                    mMessengerPtr->sendFormErrMessage(messageBuffer, messageLength, clientAddress);
                    return;
                // The query is well-formed but contains unsupported features
                case CastUtils::castEnumToInt(DnsRCodes::NOTIMP):
                    mMessengerPtr->sendNotImpMessage(messageBuffer, messageLength, clientAddress);
                    return;
                // Other parsing errors
                default:
                    mMessengerPtr->sendServFailMessage(messageBuffer, messageLength, clientAddress);
                    return;
            } // switch
        } // catch

        // The query is valid, we check if the domain is blocked
        if(mDomainFilterPtr->domainMatches(dnsQuery.mQName)) {
            logger("Domain '%s' is blocked by filter", dnsQuery.mQName.c_str());
            verbose("Blocked query for domain: %s", dnsQuery.mQName.c_str());
            mMessengerPtr->sendRefusedMessage(messageBuffer, messageLength, clientAddress, dnsQuery);
            return;
        }
        logger("Domain '%s' allowed, forwarding to upstream resolver", dnsQuery.mQName.c_str());

        // We forward the query to the resolver, if the domain is not blocked
        mForwarderPtr->forwardQueryToResolver(messageBuffer, messageLength, clientAddress);
    } // UdpFsm::onClientDatagram

    void UdpFsm::onResolverDatagram(uint8_t *messageBuffer, const size_t messageLength) const {
        logger("Processing resolver response (%zu bytes)", messageLength);

        // First, we must map the response back to the original client
        sockaddr_in clientAddress{};
        if(!mForwarderPtr->mapResponseFromResolver(messageBuffer, messageLength, clientAddress)) {
            logger("No client mapping found for resolver response (orphaned response)");
            return;
        }

        logger("Mapped response to client %s:%d, forwarding reply",
               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // Then we send the response back to the client
        mMessengerPtr->sendDnsReply(CastUtils::castByteArrayToVector(messageBuffer, messageLength), clientAddress);
    } // UdpFsm::onResolverDatagram

    void UdpFsm::transactionsMaintenance() {
        const auto now = chrono::steady_clock::now();  // current time

        // Check if it's time for maintenance
        if(now < mNextMaintenanceTimestamp) {
            return;
        }

        // Maintenance tasks - delete old transactions and set next maintenance timestamp
        mForwarderPtr->deleteOldTransactions();
        mNextMaintenanceTimestamp = now + MAINTENANCE_INTERVAL;
    } // UdpFsm::transactionsMaintenance
} // FilteringDnsResolver::Networking

/*** end of file UdpFsm.cpp ***/
