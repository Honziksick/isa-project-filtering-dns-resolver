/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         DnsHeader.cpp                                                 *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      02.10.2025                                                    *
 * Last edit:    02.10.2025                                                    *
 *                                                                             *
 * Description:  This source file implements the `DnsHeader` class, which      *
 *               represents and manipulates DNS message headers according to   *
 *               RFC 1035. It provides methods for extracting and checking     *
 *               individual flag bits and fields within the DNS header         *
 *               structure.                                                    *
 *                                                                             *
 ******************************************************************************/
/**
 * @file DnsHeader.cpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Source file implementing the `DnsHeader` class for DNS message header
 *        representation and manipulation.
 *
 * @note This implementation was inspired by (primarily section 2):
 *       `https://medium.com/@s12deff/command-and-control-c2-dns-server-part-i-d662a6764aff`
 */

#include "DnsUtils/DnsHeader.hpp"
#include <cstdint>  // uint16_t

namespace FilteringDnsResolver::DnsUtils
{
    DnsHeader::DnsHeader(const uint16_t id, const uint16_t flags, const uint16_t qdCount)
        : mId{id},
          mFlags{flags},
          mQdCount{qdCount} {}

    bool DnsHeader::isQRSet(const uint16_t flags) {
        return ((flags & QR_MASK) != 0);
    } // DnsHeader::isQRSet

    bool DnsHeader::isRDSet(const uint16_t flags) {
        return ((flags & RD_MASK) != 0);
    } // DnsHeader::isRDSet

    uint16_t DnsHeader::getOpcode(const uint16_t flags) {
        return static_cast<uint16_t>((flags & OPCODE_MASK) >> OPCODE_SHIFT);
    } // DnsHeader::getOpcode

    bool DnsHeader::isZBitZero(const uint16_t flags) {
        return ((flags & Z_MASK) == 0);
    } // DnsHeader::isZBitZero
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsHeader.cpp ***/
