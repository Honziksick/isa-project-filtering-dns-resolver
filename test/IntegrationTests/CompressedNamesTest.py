################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         CompressedNamesTest.py                                         #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  Integration tests focused on DNS name compression edge cases.  #
#               These tests verify correct handling of compressed QNAMEs,      #
#               pointer loops, out-of-bounds and malformed pointers,           #
#               multi-question packets, and RFC 1035 compliance.               #
#               The goal is to ensure the resolver robustly detects and        #
#               responds to malformed compression, correctly blocks domains,   #
#               and always echoes the first Question unchanged.                #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import pytest
import struct
import random
import time
from DnsClient import DNSTestClient, DNSRCode


# --------------------------------------------------------------------------- #
# DNS wire helpers
# --------------------------------------------------------------------------- #

HEADER_LEN = 12

def be16(x: int) -> bytes:
    """Pack an unsigned 16-bit integer in big-endian (network) order."""
    return struct.pack("!H", x)


def make_header(txid: int, flags: int = 0x0100, qd: int = 1, an: int = 0, ns: int = 0, ar: int = 0) -> bytes:
    """Build a DNS header (12 bytes)."""
    return struct.pack("!HHHHHH", txid, flags, qd, an, ns, ar)


def encode_qname(domain: str) -> bytes:
    """
    Encode a domain name to DNS QNAME wire format.

    Returns:
        bytes: The encoded QNAME (labels + zero root).
    Notes:
        Label length is limited to 63 bytes. The caller is responsible for the
        final absolute offsets inside the whole DNS message.
    """
    parts = domain.strip(".").split(".")
    out = bytearray()
    for lab in parts:
        b = lab.encode("ascii")
        if len(b) > 63:
            raise ValueError("label too long")
        out.append(len(b))
        out.extend(b)
    out.append(0)  # root label
    return bytes(out)


def encode_labels(labels):
    """
    Encode a list of label strings (without the trailing root).

    Returns:
        tuple[bytes, list[int]]: (encoded-bytes, list of absolute offsets)
        Offsets are relative to the start of the DNS message (HEADER_LEN base).
    """
    out = bytearray()
    offsets = []
    for lab in labels:
        offsets.append(HEADER_LEN + len(out))
        lab_b = lab.encode("ascii")
        out.append(len(lab_b))
        out.extend(lab_b)
    return bytes(out), offsets


def question_end_from_request_bytes(qname_bytes_len: int) -> int:
    """Compute the offset immediately after QCLASS, given QNAME length."""
    return HEADER_LEN + qname_bytes_len + 4  # QNAME + QTYPE(2) + QCLASS(2)


def parse_question_end_in_response(wire: bytes) -> int:
    """
    Robustly find the end of the Question section in a response.

    Handles pointers and guards against loops. Returns the offset just past QCLASS.
    """
    off = HEADER_LEN
    seen = set()
    jumps = 0
    MAX_JUMPS = 64

    # walk QNAME
    while True:
        if off >= len(wire):
            raise ValueError("response truncated in QNAME")

        ln = wire[off]
        if ln == 0:
            off += 1
            break

        top2 = ln & 0xC0
        if top2 == 0xC0:
            # compression pointer
            if off + 1 >= len(wire):
                raise ValueError("truncated pointer")
            ptr = ((ln & 0x3F) << 8) | wire[off + 1]
            if ptr in seen or jumps > MAX_JUMPS:
                raise ValueError("pointer loop")
            # in Question QNAME, a pointer ends the name
            off += 2
            break
        elif top2 == 0x80:
            raise ValueError("invalid 10xxxxxx label")
        else:
            # ordinary label
            lablen = ln
            off += 1
            if off + lablen > len(wire):
                raise ValueError("label beyond end")
            off += lablen

    # QTYPE + QCLASS
    if off + 4 > len(wire):
        raise ValueError("truncated QTYPE/QCLASS")
    return off + 4


def end_of_question(msg: bytes, start_off: int) -> int:
    """
    Return the offset just past QCLASS of the first Question starting at start_off.

    Understands labels and compression pointers (with simple loop protection).
    """
    off = start_off
    jumps = 0
    seen_ptrs = set()

    while True:
        if off >= len(msg):
            raise ValueError("truncated name")

        ln = msg[off]
        if ln == 0:
            off += 1
            break

        top2 = ln & 0xC0
        if top2 == 0xC0:
            # pointer terminates the name
            if off + 1 >= len(msg):
                raise ValueError("truncated pointer")
            ptr = ((ln & 0x3F) << 8) | msg[off + 1]
            if ptr in seen_ptrs:
                raise ValueError("pointer loop")
            seen_ptrs.add(ptr)
            off += 2
            break
        elif top2 == 0x80:
            raise ValueError("invalid 10xxxxxx label")
        else:
            # normal label
            lablen = ln
            off += 1
            if off + lablen > len(msg):
                raise ValueError("label beyond end")
            off += lablen

        jumps += 1
        if jumps > 128:
            raise ValueError("too many labels")

    # QTYPE + QCLASS
    if off + 4 > len(msg):
        raise ValueError("truncated QTYPE/QCLASS")
    return off + 4


# --------------------------------------------------------------------------- #
# Builders for specific malformed/edge-case query packets
# --------------------------------------------------------------------------- #

def build_two_question_with_compressed_second(
        qname1: str,
        qname2_prefix: str,
        qname2_suffix_target_offset: int,
        qtype1: int = 1,
        qclass1: int = 1,
        qtype2: int = 1,
        qclass2: int = 1,
        txid: int | None = None,
        ):
    """
    Build a message with two Questions.

    Q1 = qname1 (uncompressed).
    Q2 = qname2_prefix + pointer to qname2_suffix_target_offset (inside Q1).

    Notes:
        qname2_suffix_target_offset must point to the start of a label in Q1
        so that a valid suffix (e.g. "example.com") forms from that position.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    # Q1
    q1 = encode_qname(qname1)
    q1_start = HEADER_LEN  # Q1 starts immediately after header

    # verify pointer target lies inside Q1
    if not (q1_start <= qname2_suffix_target_offset < q1_start + len(q1)):
        raise ValueError("target offset for compression must be inside Q1")

    # Q2: prefix + pointer
    prefix_bytes = bytearray()
    for lab in qname2_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            if len(b) > 63:
                raise ValueError("label too long")
            prefix_bytes.append(len(b))
            prefix_bytes.extend(b)

    ptr_off = qname2_suffix_target_offset
    if ptr_off > 0x3FFF:
        raise ValueError("pointer offset too big")
    ptr_hi = 0xC0 | ((ptr_off >> 8) & 0x3F)
    ptr_lo = ptr_off & 0xFF
    q2 = bytes(prefix_bytes + bytes([ptr_hi, ptr_lo]))

    # Header with QDCOUNT=2
    header = make_header(txid, flags=0x0100, qd=2)

    wire = bytearray()
    wire.extend(header)
    wire.extend(q1)
    wire.extend(be16(qtype1))
    wire.extend(be16(qclass1))
    q2_start = len(wire)  # for completeness
    wire.extend(q2)
    wire.extend(be16(qtype2))
    wire.extend(be16(qclass2))
    return bytes(wire), q1_start, q2_start


def build_two_question_with_compressed_q2(
        q1_name: str,
        q2_prefix: str,
        q2_suffix_offset_in_msg: int,
        qtype1: int = 1,
        qclass1: int = 1,
        qtype2: int = 1,
        qclass2: int = 1,
        txid: int | None = None,
        ) -> bytes:
    """
    Build a DNS query with QDCOUNT=2, where Q2 = <q2_prefix> + PTR(q2_suffix_offset_in_msg).

    The PTR offset must point to the START of a label (absolute within the message),
    typically somewhere inside Q1.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid, flags=0x0100, qd=2)
    wire = bytearray(header)

    # Q1 (uncompressed), e.g. "domain.com"
    q1_bytes = encode_qname(q1_name)
    q1_start = HEADER_LEN
    wire.extend(q1_bytes)
    wire.extend(be16(qtype1))
    wire.extend(be16(qclass1))

    # Q2 prefix (e.g., "blocked" or "www")
    prefix = bytearray()
    if q2_prefix:
        b = q2_prefix.encode("ascii")
        if len(b) > 63:
            raise ValueError("prefix too long")
        prefix.append(len(b))
        prefix.extend(b)

    # pointer to suffix in Q1 (e.g., label length byte of "domain")
    if q2_suffix_offset_in_msg > 0x3FFF:
        raise ValueError("pointer offset too big for DNS compression")
    ptr_hi = 0xC0 | ((q2_suffix_offset_in_msg >> 8) & 0x3F)
    ptr_lo = q2_suffix_offset_in_msg & 0xFF
    q2_name = bytes(prefix + bytes([ptr_hi, ptr_lo]))

    wire.extend(q2_name)
    wire.extend(be16(qtype2))
    wire.extend(be16(qclass2))
    return bytes(wire)


def build_two_question_with_pointer_mid_label(
        qname1: str,
        q2_prefix: str,
        mid_label_offset: int,
        qtype1: int = 1,
        qclass1: int = 1,
        qtype2: int = 1,
        qclass2: int = 1,
        txid: int | None = None,
        ):
    """
    Q1 = qname1 (uncompressed).
    Q2 = q2_prefix + PTR to the *middle* of a label in Q1 (malformed).
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid=txid, flags=0x0100, qd=2)
    wire = bytearray(header)

    q1 = encode_qname(qname1)
    wire.extend(q1)
    wire.extend(be16(qtype1))
    wire.extend(be16(qclass1))

    # Q2 prefix
    for lab in q2_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)

    # PTR to mid_label_offset (not on a label boundary)
    ptr_off = mid_label_offset
    ptr_hi = 0xC0 | ((ptr_off >> 8) & 0x3F)
    ptr_lo = ptr_off & 0xFF
    wire.extend(bytes([ptr_hi, ptr_lo]))
    wire.extend(be16(qtype2))
    wire.extend(be16(qclass2))
    return bytes(wire)


def build_query_qname_backward_selfloop(qtype: int = 1, qclass: int = 1, txid: int | None = None) -> bytes:
    """
    Build a single-question query where QNAME contains a *backward* pointer
    to the start of its own name (e.g., 'alpha' + PTR → start of 'alpha').
    This creates a decoding loop → expected FORMERR.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid=txid, flags=0x0100, qd=1)
    wire = bytearray(header)

    qname_start = len(wire)  # equals HEADER_LEN
    label = b"alpha"
    wire.append(len(label))
    wire.extend(label)
    # pointer back to qname_start
    wire.append(0xC0 | ((qname_start >> 8) & 0x3F))
    wire.append(qname_start & 0xFF)
    # no terminal 0x00 — compression substitutes it; still a loop → FORMERR
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    return bytes(wire)


def build_two_question_pointer_to_root(
        q1_name: str,
        q2_prefix: str,
        qtype1: int = 1,
        qclass1: int = 1,
        qtype2: int = 1,
        qclass2: int = 1,
        txid: int | None = None,
        ):
    """
    Q1 = q1_name.
    Q2 = q2_prefix + PTR to the *zero* root label (the last byte of Q1 is 0x00).
    Resulting name is valid (ends at root).
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid=txid, flags=0x0100, qd=2)
    wire = bytearray(header)

    q1_start = len(wire)
    q1 = encode_qname(q1_name)
    wire.extend(q1)
    wire.extend(be16(qtype1))
    wire.extend(be16(qclass1))

    # root label offset: q1_start + len(q1) - 1
    root_off = q1_start + len(q1) - 1

    # Q2 = prefix + pointer to root
    for lab in q2_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)
    wire.append(0xC0 | ((root_off >> 8) & 0x3F))
    wire.append(root_off & 0xFF)
    wire.extend(be16(qtype2))
    wire.extend(be16(qclass2))

    return bytes(wire)


def build_three_question_valid_chain(
        q1_name: str,
        q2_prefix: str,
        q3_prefix: str,
        qtype: int = 1,
        qclass: int = 1,
        txid: int | None = None,
        ):
    """
    No loops:

    Q1 = q1_name
    Q2 = q2_prefix + PTR → start of Q1
    Q3 = q3_prefix + PTR → start of Q2
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid=txid, flags=0x0100, qd=3)
    wire = bytearray(header)

    # Q1
    q1_start = len(wire)
    q1 = encode_qname(q1_name)
    wire.extend(q1)
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    # Q2
    q2_start = len(wire)
    for lab in q2_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)
    wire.append(0xC0 | ((q1_start >> 8) & 0x3F))
    wire.append(q1_start & 0xFF)
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    # Q3
    for lab in q3_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)
    wire.append(0xC0 | ((q2_start >> 8) & 0x3F))
    wire.append(q2_start & 0xFF)
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    return bytes(wire)


def build_three_question_forward_pointer(
        q1_name: str,
        q2_prefix: str,
        q3_prefix: str,
        qtype: int = 1,
        qclass: int = 1,
        txid: int | None = None,
        ) -> bytes:
    """
    Q1 = q1_name
    Q2 = q2_prefix + PTR → *forward* to Q3 (forward pointer = malformed)
    Q3 = q3_prefix (no compression or possibly pointer backwards)
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    header = make_header(txid=txid, flags=0x0100, qd=3)
    wire = bytearray(header)

    # Q1
    q1 = encode_qname(q1_name)
    wire.extend(q1)
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    # Q2 (prefix + placeholder for a pointer set later to forward-reference Q3)
    q2_start = len(wire)
    for lab in q2_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)
    ptr_q2_pos = len(wire)
    wire.extend(b"\xC0\x00")  # temporary placeholder
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    # Q3 (full domain without compression)
    q3_start = len(wire)
    for lab in q3_prefix.strip(".").split("."):
        if lab:
            b = lab.encode("ascii")
            wire.append(len(b))
            wire.extend(b)
    wire.append(0)  # root
    wire.extend(be16(qtype))
    wire.extend(be16(qclass))

    # set the forward pointer in Q2 → forward to q3_start (invalid)
    wire[ptr_q2_pos] = 0xC0 | ((q3_start >> 8) & 0x3F)
    wire[ptr_q2_pos + 1] = q3_start & 0xFF

    return bytes(wire)


# --------------------------------------------------------------------------- #
# Builders for intentionally malformed compressed QNAMEs
# --------------------------------------------------------------------------- #

def build_query_qname_pointer_to_self(txid: int | None = None, qtype: int = 1, qclass: int = 1):
    """
    QNAME: <2>'aa' + a pointer that targets its own first pointer byte (self-loop).
    Expected: FORMERR.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)

    time.sleep(1)

    qname = bytearray()
    qname.extend(b"\x02aa")  # 'aa'
    # Absolute position of the first pointer byte in the message:
    ptr_pos_in_msg = HEADER_LEN + len(qname)
    # 14-bit offset pointing to itself
    if ptr_pos_in_msg > 0x3FFF:
        raise RuntimeError("offset too large for test")
    ptr_hi = 0xC0 | ((ptr_pos_in_msg >> 8) & 0x3F)
    ptr_lo = ptr_pos_in_msg & 0xFF
    qname.extend(bytes([ptr_hi, ptr_lo]))

    header = make_header(txid, 0x0100, qd=1)
    return header + bytes(qname) + be16(qtype) + be16(qclass), question_end_from_request_bytes(len(qname))


def build_query_qname_pointer_oob(txid: int | None = None, qtype: int = 1, qclass: int = 1):
    """
    QNAME: <1>'a' + pointer to an obviously out-of-bounds offset (0x3FFF).
    Expected: FORMERR.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)
    qname = bytearray()
    qname.extend(b"\x01a")
    qname.extend(b"\xFF\xFF")  # 0b11 11111111 -> 14-bit offset = 0x3FFF
    header = make_header(txid, 0x0100, qd=1)
    return header + bytes(qname) + be16(qtype) + be16(qclass), question_end_from_request_bytes(len(qname))


def build_query_qname_pointer_into_header(txid: int | None = None, qtype: int = 1, qclass: int = 1):
    """
    QNAME: <1>'a' + pointer to offset 0x0000 (start of header – invalid).
    Expected: FORMERR.
    """
    if txid is None:
        txid = random.randint(0, 0xFFFF)
    qname = bytearray()
    qname.extend(b"\x01a")
    qname.extend(b"\xC0\x00")  # pointer to the very beginning of the message
    header = make_header(txid, 0x0100, qd=1)
    return header + bytes(qname) + be16(qtype) + be16(qclass), question_end_from_request_bytes(len(qname))


# --------------------------------------------------------------------------- #
# Self-contained tests
# --------------------------------------------------------------------------- #

class TestCompressedNames:
    def test_compressed_qname_pointer_self_returns_formerr_and_no_crash(self, resolver_manager):
        """QNAME has a self-pointer (loop) → expect FORMERR (and no crash)."""
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        time.sleep(1)

        wire, _ = build_query_qname_pointer_to_self(qtype=1, qclass=1)
        resp = client.send_malformed_query(wire)
        assert resp is not None, "Resolver did not respond"
        assert resp.rcode == DNSRCode.FORMERR, f"Expected FORMERR, got {resp.rcode}"

    def test_compressed_qname_pointer_oob_returns_formerr(self, resolver_manager):
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        time.sleep(1)

        wire, _ = build_query_qname_pointer_oob()
        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

    def test_compressed_qname_pointer_into_header_returns_formerr(self, resolver_manager):
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[])
        client = DNSTestClient(server_port=resolver_manager.port)

        time.sleep(1)

        wire, _ = build_query_qname_pointer_into_header()
        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

    def test_notimp_precedence_with_compressed_qname_is_still_formerr(self, resolver_manager):
        """
        If QNAME is malformed (e.g., self-pointer), FORMERR takes precedence even for QTYPE=AAAA.
        """
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[])
        client = DNSTestClient(server_port=resolver_manager.port)

        time.sleep(1)

        wire, _ = build_query_qname_pointer_to_self(qtype=28, qclass=1)  # AAAA
        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

    def test_compressed_block_qdcount2_refused_and_echoes_q1(self, resolver_manager):
        """
        Q1:  blocked-domain.com
        Q2:  sub.(PTR → 'blocked-domain.com' within Q1)

        QDCOUNT>1 support: if *any* Question targets a blocked domain → expect REFUSED.
        Also verify the server echoes Question(1) unchanged.
        """
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire, _, _ = build_two_question_with_compressed_second(
                qname1="blocked-domain.com",
                qname2_prefix="sub",
                qname2_suffix_target_offset=HEADER_LEN,
                qtype1=1,
                qclass1=1,
                qtype2=1,
                qclass2=1,
                )

        time.sleep(1)

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.REFUSED, "Expected REFUSED when any QNAME is blocked"

        req_q1_end = end_of_question(wire, HEADER_LEN)
        resp_q1_end = end_of_question(resp.raw_data, HEADER_LEN)
        assert wire[HEADER_LEN:req_q1_end] == resp.raw_data[HEADER_LEN:resp_q1_end], \
            "Server should echo original Question(1) unchanged"

    def test_compressed_forward_qdcount2_no_local_error_and_echoes_q1(self, resolver_manager):
        """
        Q1: example.com
        Q2: www.(PTR → 'example.com' within Q1)

        QDCOUNT>1 is allowed → no local error (REFUSED/NOTIMP/FORMERR) expected.
        Echo of Question(1) must match.
        """
        resolver_manager.start_resolver(exact_domains=["blocked-domain.com"], wildcard_patterns=[])
        client = DNSTestClient(server_port=resolver_manager.port)

        wire, _, _ = build_two_question_with_compressed_second(
                qname1="example.com",
                qname2_prefix="www",
                qname2_suffix_target_offset=HEADER_LEN,
                qtype1=1,
                qclass1=1,
                qtype2=1,
                qclass2=1,
                )

        time.sleep(1)

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode not in (DNSRCode.REFUSED, DNSRCode.NOTIMP, DNSRCode.FORMERR), \
            "QDCOUNT>1 must not be rejected; no local error expected"

        req_q1_end = end_of_question(wire, HEADER_LEN)
        resp_q1_end = end_of_question(resp.raw_data, HEADER_LEN)
        assert wire[HEADER_LEN:req_q1_end] == resp.raw_data[HEADER_LEN:resp_q1_end], \
            "Server should echo original Question(1) unchanged"

    def test_compressed_block_refused(self, resolver_manager):
        """
        Q1: 'domain.com'
        Q2: 'blocked' + PTR → (offset of 'domain.com' in Q1)

        Expect: REFUSED (blocking 'blocked.domain.com').
        """
        resolver_manager.start_resolver(exact_domains=["blocked.domain.com"], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        txid = random.randint(0, 0xFFFF)

        wire, q1_start, q2_start = build_two_question_with_compressed_second(
                qname1="domain.com",
                qname2_prefix="blocked",
                qname2_suffix_target_offset=HEADER_LEN,
                qtype1=1,
                qclass1=1,
                qtype2=1,
                qclass2=1,
                txid=txid,
                )

        # sanity: pointer in Q2 must be C0 0C
        prefix_len = sum(len(lab) + 1 for lab in "blocked".split("."))
        ptr_hi = wire[q2_start + prefix_len]
        ptr_lo = wire[q2_start + prefix_len + 1]
        assert (ptr_hi & 0xC0) == 0xC0 and ptr_hi == 0xC0 and ptr_lo == HEADER_LEN, \
            f"Bad pointer in Q2: {ptr_hi:02x} {ptr_lo:02x} (expected C0 0C)"

        resp = client.send_malformed_query(wire)
        assert resp is not None, "Resolver did not respond"
        assert resp.transaction_id == txid, "TXID mismatch"
        assert resp.rcode == DNSRCode.REFUSED, f"Expected REFUSED, got {resp.rcode}"

        req_q1_end = end_of_question(wire, HEADER_LEN)
        resp_q1_end = end_of_question(resp.raw_data, HEADER_LEN)
        assert wire[HEADER_LEN:req_q1_end] == resp.raw_data[HEADER_LEN:resp_q1_end], \
            "Echo Question(1) must be unchanged"

    def test_compressed_forwarded_not_local_error(self, resolver_manager):
        """
        Q1: 'domain.com'
        Q2: 'www.domain.com' (www + PTR to 'domain.com')

        Not blocked → no local error expected.
        """
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire, _, _ = build_two_question_with_compressed_second(
                qname1="domain.com",
                qname2_prefix="www",
                qname2_suffix_target_offset=HEADER_LEN,
                qtype1=1,
                qclass1=1,
                qtype2=1,
                qclass2=1,
                )

        time.sleep(1)

        resp = client.send_malformed_query(wire)
        assert resp is not None, "Resolver did not respond"
        assert resp.rcode not in (DNSRCode.REFUSED, DNSRCode.NOTIMP, DNSRCode.FORMERR), \
            f"No local error expected, but got {resp.rcode}"

    def test_pointer_mid_label_offset_is_formerr(self, resolver_manager):
        """Pointer targets the MIDDLE of a label (not a boundary) → must be FORMERR."""
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        # Q1 = "domain.com"
        _q1 = encode_qname("domain.com")
        # offset HEADER_LEN+1 → points to the letter 'd' (not a label start)
        bad_target = HEADER_LEN + 1

        wire = build_two_question_with_pointer_mid_label(
                qname1="domain.com",
                q2_prefix="www",
                mid_label_offset=bad_target,
                qtype1=1,
                qclass1=1,
                qtype2=1,
                qclass2=1,
                )

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

    def test_pointer_to_root_label_valid_no_local_error(self, resolver_manager):
        """
        Q2 = 'sub.' (pointer to the root 0x00 in Q1) → valid name ending at root.
        No local error expected.
        """
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire = build_two_question_pointer_to_root(
                q1_name="example.com", q2_prefix="sub", qtype1=1, qclass1=1, qtype2=1, qclass2=1
                )

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode not in (DNSRCode.REFUSED, DNSRCode.NOTIMP, DNSRCode.FORMERR)

    def test_long_valid_chain_no_loop_no_local_error(self, resolver_manager):
        """
        Multi-step valid compression without a loop (Q3 → suffix in Q2 → suffix in Q1) → OK.
        No local error expected.
        """
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire = build_three_question_valid_chain(
                q1_name="a.example.com",   # suffix 1
                q2_prefix="x",             # Q2 = x.a.example.com
                q3_prefix="y.x",           # Q3 = y.x.a.example.com (via pointers into Q2/Q1)
                qtype=1,
                qclass=1,
                )

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode not in (DNSRCode.REFUSED, DNSRCode.NOTIMP, DNSRCode.FORMERR)

    def test_forward_pointer_between_questions_is_formerr(self, resolver_manager):
        """
        Q2 contains a forward pointer (to Q3) → forward pointer is invalid → FORMERR.
        (No loop tested here; only forward-pointer rejection is validated.)
        """
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire = build_three_question_forward_pointer(
                q1_name="example.com", q2_prefix="alpha", q3_prefix="beta", qtype=1, qclass=1
                )

        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

    def test_backward_self_reference_loop_is_formerr(self, resolver_manager):
        """
        Single Question; QNAME has a *backward* pointer to the start of its own name
        (e.g., 'alpha' + PTR → start of 'alpha'). No forward pointer used.
        Expected: FORMERR (infinite decoding loop).
        """
        resolver_manager.start_resolver(exact_domains=[], wildcard_patterns=[], verbose=True)
        client = DNSTestClient(server_port=resolver_manager.port)

        wire = build_query_qname_backward_selfloop(qtype=1, qclass=1)
        resp = client.send_malformed_query(wire)
        assert resp is not None
        assert resp.rcode == DNSRCode.FORMERR

### end of file CompressedNamesTest.py ###
