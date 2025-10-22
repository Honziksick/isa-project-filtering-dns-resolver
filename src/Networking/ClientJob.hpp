/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         ClientJob.hpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      12.10.2025                                                    *
 * Last edit:    17.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides the `ClientJob` class, which        *
 *               encapsulates a DNS message received from a client, including  *
 *               the message buffer, its length, and the client's socket       *
 *               address. It is used for passing DNS jobs between threads      *
 *               within the UDP finite state machine of the filtering DNS      *
 *               resolver.                                                     *
 *                                                                             *
 ******************************************************************************/
/**
 * @file ClientJob.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring the `ClientJob` class for encapsulating DNS
 *        jobs with message data and client address information.
 */

#ifndef CLIENT_JOB_HPP
#define CLIENT_JOB_HPP

#include <netinet/in.h>  // sockaddr_in
#include <cstdint>       // uint8_t
#include <array>         // std::array

namespace FilteringDnsResolver::Networking
{
    /**
     * @class ClientJob
     * @brief Represents a DNS job containing message data and client address.
     *
     * @details Encapsulates the DNS message buffer, its length, and the
     *          socket address of the requesting client for processing
     *          within the UDP finite state machine.
     */
    class ClientJob final {
    public:
        std::array<uint8_t, 512> mMessage;  /**< Buffer holding the DNS message data. */
        size_t mMessageLength;       /**< Length of the DNS message in bytes.      */
        sockaddr_in mClientAddress;  /**< Socket address of the requesting client. */
    }; // ClientJob
} // FilteringDnsResolver::Networking

#endif // CLIENT_JOB_HPP

/*** end of file ClientJob.hpp ***/
