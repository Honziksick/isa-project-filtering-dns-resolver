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
#include "Enums/DnsOpcodes.hpp"
#include "Utilities/CastUtils.hpp"
#include "Utilities/Logger.hpp"
#include <cstdint>  // uint16_t

using namespace FilteringDnsResolver::Enums;
using namespace FilteringDnsResolver::Utilities;

namespace FilteringDnsResolver::DnsUtils
{
    DnsHeader::DnsHeader(const uint16_t id, const uint16_t flags, const uint16_t qdCount)
        : mId{id},
          mFlags{flags},
          mQdCount{qdCount} {
        logger("DnsHeader constructor: created header with ID=%u (0x%04X), "
               "flags=0x%04X, qdCount=%u", id, id, flags, qdCount);
        verbose("Processing DNS header with transaction ID %u", id);
    } // DnsHeader::DnsHeader

    bool DnsHeader::isQRSet(const uint16_t flags) {
        logger("DnsHeader::isQRSet() called with flags=0x%04X", flags);
        const bool qrBit = ((flags & QR_MASK) != 0);
        logger("QR bit check: flags=0x%04X & QR_MASK=0x%04X = %s",
               flags, QR_MASK, qrBit ? "SET (response)" : "CLEAR (query)");
        return qrBit;
    } // DnsHeader::isQRSet

    bool DnsHeader::isRDSet(const uint16_t flags) {
        logger("DnsHeader::isRDSet() called with flags=0x%04X", flags);
        const bool rdBit = ((flags & RD_MASK) != 0);
        logger("RD bit check: flags=0x%04X & RD_MASK=0x%04X = %s",
               flags, RD_MASK, rdBit ? "SET (recursion desired)" : "CLEAR (no recursion)");
        return rdBit;
    } // DnsHeader::isRDSet

    uint16_t DnsHeader::getOpcode(const uint16_t flags) {
        logger("DnsHeader::getOpcode() called with flags=0x%04X", flags);
        const auto opcode = static_cast<uint16_t>((flags & OPCODE_MASK) >> OPCODE_SHIFT);
        logger("Opcode extraction: (flags=0x%04X & OPCODE_MASK=0x%04X) >> %u = %u",
               flags, OPCODE_MASK, OPCODE_SHIFT, opcode);

        logger("Opcode %u identified as %s", opcode,
               CastUtils::castEnumToString<DnsOpcodes>(CastUtils::castIntToEnum<DnsOpcodes>(opcode)).c_str());

        return opcode;
    } // DnsHeader::getOpcode

    bool DnsHeader::isZBitZero(const uint16_t flags) {
        logger("DnsHeader::isZBitZero() called with flags=0x%04X", flags);
        const bool zBitZero = ((flags & Z_MASK) == 0);
        logger("Z bit check: flags=0x%04X & Z_MASK=0x%04X = %s",
               flags, Z_MASK, zBitZero ? "ZERO (valid)" : "NON-ZERO (invalid)");
        return zBitZero;
    } // DnsHeader::isZBitZero
} // FilteringDnsResolver::DnsUtils

/*** end of file DnsHeader.cpp ***/
