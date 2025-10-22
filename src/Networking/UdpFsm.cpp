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
 * Last edit:    13.10.2025                                                    *
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
#include "Networking/ClientJob.hpp"
#include "DnsUtils/DnsMessageParser.hpp"
#include "DnsUtils/DnsMessenger.hpp"
#include "DnsUtils/DnsForwarder.hpp"
#include "DnsUtils/DnsQuery.hpp"
#include "Constants/CustomLimits.hpp"
#include "Enums/DnsRCodes.hpp"
#include "Exceptions/CustomExceptions.hpp"
#include "Utilities/Logger.hpp"
#include "Utilities/SignalHandler.hpp"
#include "Utilities/CastUtils.hpp"
#include "Utilities/TSQueue.hpp"
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
#include <thread>        // std::thread, std::this_thread::sleep_for
#include <vector>        // std::vector
#include <mutex>         // std::mutex, std::lock_guard
#include <atomic>        // std::atomic

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
          mNextMaintenanceTimestamp{chrono::steady_clock::now() + MAINTENANCE_INTERVAL_MS} {
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

    UdpFsm::~UdpFsm() {
        try {
            stopLisResThreads();
            stopWorkerThreads();
        }
        catch(...) {
            // best-effort
        }
    } // UdpFsm::~UdpFsm

    void UdpFsm::run() {
        logger("UdpFsm::run(): starting worker pool and Listener/Resolver threads");
        verbose("DNS resolver is now listening for queries...");

        // Start worker threads and Listener/Resolver threads
        startWorkerThreads();
        startLisResThreads();

        // Main FSM loop with periodic maintenance
        while(mAreLisResThreadsRunning.load()) {
            SignalHandler::checkSignals();
            transactionsMaintenance();
            this_thread::sleep_for(chrono::milliseconds(MAIN_LOOP_SLEEP_MS));
        }
    } // UdpFsm::run

    void UdpFsm::startLisResThreads() {
        // Check if Listener/Resolver threads are already running
        if(mAreLisResThreadsRunning.exchange(true)) {
            logger("startLisResThreads(): Listener/Resolver threads already running");
            return;
        }
        logger("Starting Listener/Resolver threads");
        verbose("Initializing dedicated Listener/Resolver threads");

        // Start listener thread
        mListenerThread = thread([this] {
            listenerLoop();
        });
        logger("Listener thread started");

        // Start resolver thread
        mResolverThread = thread([this] {
            resolverLoop();
        });
        logger("Resolver thread started");
    } // UdpFsm::startLisResThreads

    void UdpFsm::stopLisResThreads() {
        if(!mAreLisResThreadsRunning.exchange(false)) {
            logger("stopLisResThreads(): Listener/Resolver threads already stopped");
            return;
        }
        logger("Stopping Listener/Resolver threads...");

        // Join threads
        if(mListenerThread.joinable()) {
            mListenerThread.join();
        }
        if(mResolverThread.joinable()) {
            mResolverThread.join();
        }

        logger("Listener/Resolver threads stopped");
    } // UdpFsm::stopLisResThreads

    void UdpFsm::startWorkerThreads() {
        if(mAreWorkerThreadsRunning.exchange(true)) {
            logger("startWorkerThreads(): workers already running");
            return;
        }
        logger("Starting %d worker threads", WORKER_THREAD_COUNT);
        verbose("Initializing worker pool for client datagrams");

        // Start worker threads
        mWorkerThreads.reserve(WORKER_THREAD_COUNT);
        for(int iThread = 0; iThread < WORKER_THREAD_COUNT; iThread++) {
            mWorkerThreads.emplace_back([this, iThread] {
                workerLoop(static_cast<size_t>(iThread));
            });

            logger("Worker thread #%d started (TID active)", iThread);
        } // for
    } // UdpFsm::startWorkerThreads

    void UdpFsm::stopWorkerThreads() {
        // Check if workers are still running
        if(!mAreWorkerThreadsRunning.exchange(false)) {
            logger("stopWorkerThreads(): workers already stopped");
            return;
        }

        logger("Stopping worker threads...");

        // Close the client queue to signal workers to exit
        mClientQueue.close();
        for(auto &workerThread : mWorkerThreads) {
            if(workerThread.joinable()) {
                workerThread.join();
            }
        }

        // Clear the worker threads vector
        mWorkerThreads.clear();
        logger("All worker threads joined successfully");
    } // UdpFsm::stopWorkerThreads

    void UdpFsm::setupListenerPollFd(pollfd &fdWatcher) const {
        // Listener socket
        fdWatcher.fd = mSocketFds->getListenerSocketFd();
        fdWatcher.events = POLLIN;
    } // UdpFsm::setupListenerPollFd

    void UdpFsm::setupResolverPollFd(pollfd &fdWatcher) const {
        // Resolver socket
        fdWatcher.fd = mSocketFds->getResolverSocketFd();
        fdWatcher.events = POLLIN;
    } // UdpFsm::setupResolverPollFd

    int UdpFsm::pollEvents(pollfd &fdWatcher) {
        const int eventCount = poll(&fdWatcher, POLL_FD_COUNT_IN_ONE_THREAD, POLL_TIMEOUT_MS);
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

    void UdpFsm::workerLoop(const size_t workerIndex) {
        logger("workerLoop(): worker #%zu entering processing loop", workerIndex);

        // Main worker loop
        while(mAreWorkerThreadsRunning.load()) {
            ClientJob job{};

            // Wait for a job from the queue
            if(!mClientQueue.pop(job)) {
                logger("workerLoop(): worker #%zu exiting (queue closed)", workerIndex);
                break;
            }

            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &job.mClientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
            logger("workerLoop(): worker #%zu picked job: %zu bytes from %s:%u (queue size approx=%zu)",
                   workerIndex, job.mMessageLength, addressBuffer,
                   ntohs(job.mClientAddress.sin_port), mClientQueue.size());

            // We process the client datagram
            try {
                onClientDatagram(job.mMessage.data(), job.mMessageLength, job.mClientAddress);
            }
            catch(const BaseCustomException<ExitCodes> &e) {
                logger("workerLoop(): onClientDatagram() exception: %s", e.what());
                try {
                    mMessengerPtr->sendServFailMessage(job.mMessage.data(), job.mMessageLength, job.mClientAddress);
                }
                catch(...) {
                    logger("workerLoop(): failed to send SERVFAIL response to client");
                }
            }
            catch(...) {
                logger("workerLoop(): onClientDatagram() unknown exception");
                try {
                    mMessengerPtr->sendServFailMessage(job.mMessage.data(), job.mMessageLength, job.mClientAddress);
                }
                catch(...) {
                    logger("workerLoop(): failed to send SERVFAIL response to client");
                }
            }
        } // while(mAreWorkerThreadsRunning.load())

        logger("workerLoop(): worker #%zu terminated", workerIndex);
    } // UdpFsm::workerLoop

    void UdpFsm::listenerLoop() {
        logger("listenerLoop(): entering main loop");

        array<uint8_t, CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE> buffer{};
        logger("Allocated receive buffer of %zu bytes", buffer.size());

        // Main listener loop
        while(mAreLisResThreadsRunning.load()) {
            // Setup pollfd
            pollfd fdWatcher{};
            setupListenerPollFd(fdWatcher);

            // Wait for events
            if(pollEvents(fdWatcher) == 0) {
                continue;
            }

            // Handle incoming client datagrams
            if(fdWatcher.revents & POLLIN) {
                auto timeoutStart{chrono::steady_clock::now()};
                size_t enqueued{0};

                while(true) {
                    // Check client queue size for throttling
                    const size_t queueSize = mClientQueue.size();
                    if(queueSize >= CLIENT_QUEUE_THROTTLE_LIMIT) {
                        logger("listenerLoop(): queue watermark reached (%zu >= %zu), pausing intake",
                               queueSize, CLIENT_QUEUE_THROTTLE_LIMIT);
                        break;
                    }

                    // Receive datagram from client
                    sockaddr_in clientAddress{};
                    socklen_t clientAddressLength{sizeof(clientAddress)};
                    const ssize_t bytesReceived = recvfrom(mSocketFds->getListenerSocketFd(),
                                                           buffer.data(), buffer.size(), MSG_DONTWAIT,
                                                           reinterpret_cast<sockaddr*>(&clientAddress),
                                                           &clientAddressLength); // we act as UDP server
                    // If the recvfrom() function returns an error
                    if(bytesReceived < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            logger("listenerLoop(): buffer drained after %zu enqueued packets", enqueued);
                            break; // No more data
                        }
                        logger("ERROR: recvfrom() failed errno=%d: %s", errno, strerror(errno));
                        throw ProtocolErrorException("UDP recvfrom() error: " + string(strerror(errno)));
                    }
                    // If the recvfrom() function returns 0, it means the connection has been closed
                    if(bytesReceived == 0) {
                        logger("listenerLoop(): recvfrom() returned 0 (closed?)");
                        continue;
                    }

                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
                    logger("listenerLoop(): received %zd bytes from client %s:%d",
                           bytesReceived, addressBuffer, ntohs(clientAddress.sin_port));

                    // We enqueue the received datagram for processing
                    ClientJob job{};
                    copy_n(buffer.data(), bytesReceived, job.mMessage.begin());
                    job.mMessageLength = static_cast<size_t>(bytesReceived);
                    job.mClientAddress = clientAddress;

                    // Enqueue job
                    const size_t approxBefore = queueSize;
                    mClientQueue.push(move(job));
                    enqueued++;

                    logger("listenerLoop(): enqueued job (approx queue size before=%zu, after~=%zu)",
                           approxBefore, approxBefore + 1);

                    if(chrono::steady_clock::now() - timeoutStart >= LISTENER_READ_TIMEOUT_MS) {
                        logger("listenerLoop(): time budget reached after %zu enqueued packets", enqueued);
                        break;
                    }
                } // while(true)
            } // if(listener POLLIN)
        } // while(mIoRunning.load())
        logger("listenerLoop(): terminated");
    } // UdpFsm::listenerLoop

    void UdpFsm::resolverLoop() {
        logger("resolverLoop(): entering main loop");

        array<uint8_t, CustomLimits::MAX_DNS_UDP_MESSAGE_SIZE> buffer{};
        logger("Allocated receive buffer of %zu bytes", buffer.size());

        // Main resolver loop
        while(mAreLisResThreadsRunning.load()) {
            // Setup pollfd
            pollfd fdWatcher{};
            setupResolverPollFd(fdWatcher);

            // Wait for events
            if(pollEvents(fdWatcher) == 0) {
                continue;
            }

            // Handle incoming resolver responses
            if(fdWatcher.revents & POLLIN) {
                auto timeoutStart{chrono::steady_clock::now()};
                size_t packetsProcessed{0};

                // Read all available datagrams from resolver
                while(true) {
                    const ssize_t bytesReceived = recv(mSocketFds->getResolverSocketFd(),
                                                       buffer.data(), buffer.size(), MSG_DONTWAIT);

                    // If the recv() function returns an error
                    if(bytesReceived < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            logger("resolverLoop(): buffer drained after %zu packets", packetsProcessed);
                            break;
                        }
                        logger("ERROR: recv() from resolver failed errno=%d: %s", errno, strerror(errno));
                        throw ConnectionErrorException(
                                "recv() from resolver error: " + string(strerror(errno))
                                );
                    }

                    // If the recv() function returns 0, it means the connection has been closed
                    if(bytesReceived == 0) {
                        logger("resolverLoop(): recv() returned 0 (closed?)");
                        continue;
                    }

                    logger("resolverLoop(): received %zd bytes from upstream resolver (packet #%zu)",
                           bytesReceived, packetsProcessed + 1);

                    try {
                        // We process the resolver datagram
                        onResolverDatagram(buffer.data(), static_cast<size_t>(bytesReceived));
                        packetsProcessed++;
                    }
                    catch(const BaseCustomException<ExitCodes> &e) {
                        logger("onResolverDatagram() exception: %s", e.what());
                        packetsProcessed++;   // failed packet still counts
                    }
                    catch(...) {
                        logger("onResolverDatagram() unknown exception");
                        packetsProcessed++;
                    }

                    if(chrono::steady_clock::now() - timeoutStart >= RESOLVER_READ_TIMEOUT_MS) {
                        logger("resolverLoop(): time budget reached after %zu packets", packetsProcessed);
                        break;
                    }
                } // while(true)
            } // if(resolver POLLIN)
        } // while(mIoRunning.load())
        logger("resolverLoop(): terminated");
    } // UdpFsm::resolverLoop

    void UdpFsm::onClientDatagram(const uint8_t *messageBuffer,
                                  const size_t messageLength,
                                  const sockaddr_in &clientAddress) const {
        char addressBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
        logger("Processing client datagram from %s:%d (%zu bytes)",
               addressBuffer, ntohs(clientAddress.sin_port), messageLength);

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

            string joined;
            for(const auto &name : dnsQuery.mQNames) {
                if(!joined.empty()) {
                    joined += ", ";
                }
                joined += name;
            }
            logger("DNS query parsed successfully: domains=[%s], qdcount=%zu",
                   joined.c_str(), dnsQuery.mQNames.size());
        }
        catch(const DnsParseErrorException &e) {
            logger("DNS parse error (code=%d): %s", e.code(), e.what());
            dnsRCode = CastUtils::castIntToEnum<DnsRCodes>(e.code());
        } // catch

        // If there was a parsing error, we respond accordingly
        switch(dnsRCode) {
            // No error, we can proceed
            case DnsRCodes::NOERROR:
                logger("No DNS parsing errors detected, proceeding with query processing");
                break;
                // The query is malformed
            case DnsRCodes::FORMERR:
                cout << "HERE" << endl << flush;
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
                logger("Validation failed: DNS parsing error with RCODE=%s",
                    CastUtils::castEnumToString<DnsRCodes>(dnsRCode).c_str());
                mMessengerPtr->sendServFailMessage(messageBuffer, messageLength, clientAddress);
                return;
        } // switch

        // We check for QNAME/QTYPE/QCLASS count consistency
        if(dnsQuery.mQNames.size() != dnsQuery.mQTypes.size() ||
            dnsQuery.mQNames.size() != dnsQuery.mQClasses.size()) {
            logger("QNAME/QTYPE/QCLASS count mismatch: qnames=%zu, qtypes=%zu, qclasses=%zu",
                   dnsQuery.mQNames.size(), dnsQuery.mQTypes.size(), dnsQuery.mQClasses.size());
            mMessengerPtr->sendFormErrMessage(messageBuffer, messageLength, clientAddress);
            return;
        }

        // The query is valid, we check if the domain is blocked)
        for(int i = 0; i < dnsQuery.mQNames.size(); i++) {
            if(mDomainFilterPtr->domainMatches(dnsQuery.mQNames[i])) {
                logger("Domain '%s' is BLOCKED -> sending REFUSED (qtype=%u, qclass=%u)",
                       dnsQuery.mQNames[i].c_str(), dnsQuery.mQTypes[i], dnsQuery.mQClasses[i]);
                verbose("Blocked query for domain: %s", dnsQuery.mQNames[i].c_str());
                mMessengerPtr->sendRefusedMessage(messageBuffer, messageLength, clientAddress, dnsQuery);
                return;
            } // if domain blocked
            else {
                logger("Domain '%s' is allowed (qtype=%u, qclass=%u)",
                       dnsQuery.mQNames[i].c_str(), dnsQuery.mQTypes[i], dnsQuery.mQClasses[i]);
            }
        } // for each QNAME

        string allQNames;
        for(const auto &name : dnsQuery.mQNames) {
            if(!allQNames.empty()) {
                allQNames += ", ";
            }
            allQNames += name;
        }
        logger("Domain(s) '%s' allowed, forwarding to upstream resolver", allQNames.c_str());
        verbose("Forwarding query for allowed domain(s): %s", allQNames.c_str());

        // We forward the query to the resolver, if the domain is not blocked
        { /* lock scope */
            lock_guard<mutex> lock(mMutex);
            mForwarderPtr->forwardQueryToResolver(messageBuffer, messageLength, clientAddress);
        }
    } // UdpFsm::onClientDatagram

    void UdpFsm::onResolverDatagram(uint8_t *messageBuffer, const size_t messageLength) const {
        logger("Processing resolver response (%zu bytes)", messageLength);

        // First, we must map the response back to the original client
        sockaddr_in clientAddress{};
        { /* lock scope */
            lock_guard<mutex> lock(mMutex);
            if(!mForwarderPtr->mapResponseFromResolver(messageBuffer, messageLength, clientAddress)) {
                logger("No client mapping found for resolver response (orphaned response)");
                return;
            }
        }

        char addressBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
        logger("Mapped response to client %s:%d, forwarding reply",
               addressBuffer, ntohs(clientAddress.sin_port));

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
        { /* lock scope */
            lock_guard<mutex> lock(mMutex);
            mForwarderPtr->deleteOldTransactions();
        }
        mNextMaintenanceTimestamp = now + MAINTENANCE_INTERVAL_MS;

        logger("Maintenance completed: next scheduled at %lld ms from now",
               chrono::duration_cast<chrono::milliseconds>(mNextMaintenanceTimestamp - now).count());
    } // UdpFsm::transactionsMaintenance
} // FilteringDnsResolver::Networking

/*** end of file UdpFsm.cpp ***/
