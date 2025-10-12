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

namespace FilteringDnsResolver::Networking {
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

                // Check if answer has arrived from the resolver
                if(fdWatcher[POLL_RESOLVER_INDEX].revents & POLLIN) {
                    logger("Processing resolver responses (max %d packets)", MAX_RESOLVER_PACKETS_PER_ITERATION);

                    int packetsProcessed{0};
                    while(packetsProcessed < MAX_RESOLVER_PACKETS_PER_ITERATION) {
                        const ssize_t bytesReceived = recv(mSocketFds->getResolverSocketFd(),
                                                           buffer.data(), buffer.size(), MSG_DONTWAIT); // we act as UDP client

                        // If the recv() function returns an error
                        if(bytesReceived < 0) {
                            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                logger("Resolver buffer drained after %d packets", packetsProcessed);
                                break;  // No more data
                            }
                            else {
                                logger("ERROR: recv() from resolver failed with errno=%d: %s",
                                       errno, strerror(errno));
                                throw ConnectionErrorException(
                                        "Failed to receive data from the server due "
                                        "to recv() error: " + string(strerror(errno))
                                        );
                            }
                        }
                        // If the recv() function returns 0, it means the connection has been closed
                        if(bytesReceived == 0) {
                            logger("recv() returned 0: resolver connection closed");
                            throw ConnectionErrorException("Connection closed by server. No data received.");
                        }

                        logger("Received %zd bytes from upstream resolver (packet %d/%d)",
                               bytesReceived, packetsProcessed + 1, MAX_RESOLVER_PACKETS_PER_ITERATION);

                        try {
                            onResolverDatagram(buffer.data(), static_cast<size_t>(bytesReceived));
                            packetsProcessed++;
                        }
                        catch(const BaseCustomException<ExitCodes> &e) {
                            logger("onResolverDatagram() exception: %s", e.what());
                            packetsProcessed++;  // failed packet still counts
                        }
                        catch(...) {
                            logger("onResolverDatagram() unknown exception");
                            packetsProcessed++;
                            // TODO: what behavior here?
                        }
                    } // while(packetsProcessed < MAX_RESOLVER_PACKETS_PER_ITERATION)

                    if(packetsProcessed >= MAX_RESOLVER_PACKETS_PER_ITERATION) {
                        logger("NOTICE: Resolver batch limit reached (%d packets), "
                               "more data may be pending", packetsProcessed);
                    }
                } // if(resolver POLLIN)

                // Check if a new datagram has arrived from a client
                if(fdWatcher[POLL_LISTENER_INDEX].revents & POLLIN) {
                    logger("Processing client queries (max %d packets)", MAX_CLIENT_PACKETS_PER_ITERATION);

                    int packetsProcessed = 0;
                    while(packetsProcessed < MAX_CLIENT_PACKETS_PER_ITERATION) {
                        sockaddr_in clientAddress{};
                        socklen_t clientAddressLength{sizeof(clientAddress)};
                        const ssize_t bytesReceived = recvfrom(mSocketFds->getListenerSocketFd(),
                                                               buffer.data(), buffer.size(), MSG_DONTWAIT,
                                                               reinterpret_cast<sockaddr*>(&clientAddress),
                                                               &clientAddressLength); // we act as UDP server

                        // If the recvfrom() function returns an error
                        if(bytesReceived < 0) {
                            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                logger("Client buffer drained after %d packets", packetsProcessed);
                                break;  // No more data
                            }
                            else {
                                logger("ERROR: recvfrom() failed with errno=%d: %s", errno, strerror(errno));
                                throw ProtocolErrorException("UDP datagram exceeds maximum size.");
                            }
                        }
                        // If the recvfrom() function returns 0, it means the connection has been closed
                        if(bytesReceived == 0) {
                            logger("recvfrom() returned 0: connection closed");
                            throw ConnectionErrorException("Connection closed by server. No data received.");
                        }

                        logger("Received %zd bytes from client %s:%d (packet %d/%d)", bytesReceived,
                               inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port),
                               packetsProcessed + 1, MAX_CLIENT_PACKETS_PER_ITERATION);

                        // We process the received datagram
                        try {
                            onClientDatagram(buffer.data(), static_cast<size_t>(bytesReceived), clientAddress);
                            packetsProcessed++;
                        }
                        catch(const BaseCustomException<ExitCodes> &e) {
                            logger("onClientDatagram() exception: %s", e.what());
                            try {
                                mMessengerPtr->sendServFailMessage(buffer.data(),
                                                                   static_cast<size_t>(bytesReceived),
                                                                   clientAddress);
                            }
                            catch(...) {
                                // TODO: what behavior here?
                                logger("Failed to send SERVFAIL response to client");
                            }
                            packetsProcessed++; // failed packet still counts
                        }
                        catch(...) {
                            logger("onClientDatagram() unknown exception");
                            try {
                                mMessengerPtr->sendServFailMessage(buffer.data(),
                                                                   static_cast<size_t>(bytesReceived),
                                                                   clientAddress);
                            }
                            catch(...) {
                                // TODO: what behavior here?
                                logger("Failed to send SERVFAIL response to client");
                            }
                            packetsProcessed++;
                        }
                    } // while(packetsProcessed < MAX_CLIENT_PACKETS_PER_ITERATION)

                    if(packetsProcessed >= MAX_CLIENT_PACKETS_PER_ITERATION) {
                        logger("WARNING: Client batch limit reached (%d packets), "
                               "more data may be pending", packetsProcessed);
                    }
                } // if(listener POLLIN)
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
        auto dnsRCode{DnsRCodes::NOERROR};

        try {
            DnsMessageParser::parseAndValidate(messageBuffer, messageLength, dnsQuery);
            logger("DNS query parsed successfully: domain='%s'", dnsQuery.mQName.c_str());
        }
        catch(const DnsParseErrorException &e) {
            logger("DNS parse error (code=%d): %s", e.code(), e.what());
            dnsRCode = CastUtils::castIntToEnum<DnsRCodes>(e.code());
        } // catch

        // The query is valid, we check if the domain is blocked
        if(mDomainFilterPtr->domainMatches(dnsQuery.mQName)) {
            logger("Domain '%s' is BLOCKED -> sending REFUSED (qtype=%u, qclass=%u)",
                   dnsQuery.mQName.c_str(), dnsQuery.mQType, dnsQuery.mQClass);
            verbose("Blocked query for domain: %s", dnsQuery.mQName.c_str());
            mMessengerPtr->sendRefusedMessage(messageBuffer, messageLength, clientAddress, dnsQuery);
            return;
        }

        // If there was a parsing error, we respond accordingly
        switch(dnsRCode) {
            // No error, we can proceed
            case DnsRCodes::NOERROR:
                logger("No DNS parsing errors detected, proceeding with query processing");
                break;
            // The query is malformed
            case DnsRCodes::FORMERR:
                logger("Validation failed: malformed DNS query");
                mMessengerPtr->sendFormErrMessage(messageBuffer, messageLength, clientAddress);
                return;
            // The query is well-formed but contains unsupported features
            case DnsRCodes::NOTIMP:
                logger("Validation failed: unsupported DNS query features");
                mMessengerPtr->sendNotImpMessage(messageBuffer, messageLength, clientAddress);
                return;
            // Other parsing errors
            default:
                logger("Validation failed: DNS parsing error with RCODE=%s", CastUtils::castEnumToString<DnsRCodes>(dnsRCode).c_str());
                mMessengerPtr->sendServFailMessage(messageBuffer, messageLength, clientAddress);
                return;
        } // switch

        // Only A IN queries (QTYPE=1, QCLASS=1) are allowed
        logger("Validating QCLASS: expected=1 (IN), actual=%u", dnsQuery.mQClass);
        if(dnsQuery.mQClass != 1) {
            logger("Validation failed: QCLASS=%u (only IN class supported)", dnsQuery.mQClass);
            verbose("Unsupported query class %u - rejecting with NOTIMP", dnsQuery.mQClass);
            mMessengerPtr->sendNotImpMessage(messageBuffer, messageLength, clientAddress, dnsQuery);
            return; // we musn't forward unsupported class
        }
        logger("QCLASS validation passed (Internet class)");

        logger("Validating QTYPE: expected=1 (A record), actual=%u", dnsQuery.mQType);
        if(dnsQuery.mQType != 1) {
            logger("Validation failed: QTYPE=%u (only A records supported)", dnsQuery.mQType);
            verbose("Unsupported query type %u - still going to forward it", dnsQuery.mQType);
        }
        else {
            logger("QTYPE validation passed (A record query)");
        }

        logger("Domain '%s' allowed, forwarding to upstream resolver", dnsQuery.mQName.c_str());
        verbose("Forwarding query for allowed domain: %s", dnsQuery.mQName.c_str());

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
            logger("Skipping maintenance: next scheduled at %lld ms from now",
                   chrono::duration_cast<chrono::milliseconds>(mNextMaintenanceTimestamp - now).count());
            return;
        }

        // Maintenance tasks - delete old transactions and set next maintenance timestamp
        mForwarderPtr->deleteOldTransactions();
        mNextMaintenanceTimestamp = now + MAINTENANCE_INTERVAL;

        logger("Maintenance completed: next scheduled at %lld ms from now",
               chrono::duration_cast<chrono::milliseconds>(mNextMaintenanceTimestamp - now).count());
    } // UdpFsm::transactionsMaintenance
} // FilteringDnsResolver::Networking

/*** end of file UdpFsm.cpp ***/
