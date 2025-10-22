################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         DnsClient.py                                                   #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  DNS test client and result models for integration tests of     #
#               the Filtering DNS Resolver. Provides methods for sending       #
#               queries, parsing responses, batch and concurrent testing,      #
#               and verifying blocking/allowing behavior. Enables flexible     #
#               and reusable DNS test scenarios for all integration modules.   #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import socket
import struct
import time
import asyncio
from typing import Optional, Dict, List, Tuple, Union
from dataclasses import dataclass, field
from enum import IntEnum

import dns.message
import dns.query
import dns.rdatatype
import dns.rcode


# =============================================================================
# RCODE enum
# =============================================================================

class DNSRCode(IntEnum):
    """DNS response codes."""
    NOERROR = 0
    FORMERR = 1
    SERVFAIL = 2
    NXDOMAIN = 3
    NOTIMP = 4
    REFUSED = 5


# =============================================================================
# Response & batch result models
# =============================================================================

@dataclass
class DNSResponse:
    """Parsed DNS response with timing and basic header fields."""
    rcode: int
    transaction_id: int
    flags: int
    questions: int
    answers: int
    authority: int
    additional: int
    raw_data: bytes
    response_time: float
    answer_records: List[str] = field(default_factory=list)
    domain_queried: str = ""
    query_type: int = 1

    def is_blocked(self) -> bool:
        """True if the response indicates policy-based blocking (REFUSED)."""
        return self.rcode == DNSRCode.REFUSED

    def is_allowed(self) -> bool:
        """True if the response indicates the query was not blocked."""
        return self.rcode in (DNSRCode.NOERROR, DNSRCode.NXDOMAIN)

    def has_answers(self) -> bool:
        """True if the response contains at least one answer record."""
        return self.answers > 0 and len(self.answer_records) > 0


@dataclass
class BatchTestResult:
    """Aggregate results for batch DNS tests."""
    total: int = 0
    blocked: int = 0
    allowed: int = 0
    errors: int = 0
    timeouts: int = 0
    results: List[Tuple[str, DNSResponse]] = field(default_factory=list)
    failed_domains: List[Tuple[str, str]] = field(default_factory=list)

    @property
    def success_rate(self) -> float:
        """Percentage of tests that did not error out."""
        if self.total == 0:
            return 0.0
        return ((self.total - self.errors) / self.total) * 100.0

    @property
    def block_rate(self) -> float:
        """Percentage of domains that were blocked."""
        if self.total == 0:
            return 0.0
        return (self.blocked / self.total) * 100.0

    def get_failure_summary(self) -> str:
        """Human-friendly summary of failed domains."""
        if not self.failed_domains:
            return "No failures"
        lines = ["Failed domains:"]
        for domain, reason in self.failed_domains:
            lines.append(f"  - {domain}: {reason}")
        return "\n".join(lines)


# =============================================================================
# DNS test client
# =============================================================================

class DNSTestClient:
    """Minimal DNS test client for exercising a resolver under test."""

    def __init__(
            self,
            server_ip: str = "127.0.0.1",
            server_port: int = 15353,
            timeout: float = 10.0,
            verbose: bool = True,
            ):
        """
        Args:
            server_ip: Resolver IP to send queries to.
            server_port: Resolver UDP port.
            timeout: Socket timeout in seconds.
            verbose: If True, print client logs.
        """
        self.server_ip = server_ip
        self.server_port = server_port
        self.timeout = timeout
        self.verbose = verbose
        self.transaction_id = 0x1234

    # ------------------------------------------------------------------ #
    # Internal helpers
    # ------------------------------------------------------------------ #

    def _log(self, message: str) -> None:
        """Log messages when verbose mode is enabled."""
        if self.verbose:
            print(f"[DNSTestClient] {message}")

    # ------------------------------------------------------------------ #
    # Wire-level helpers
    # ------------------------------------------------------------------ #

    def create_dns_query(self, domain: str, qtype: int = 1, qclass: int = 1) -> bytes:
        """Build a raw DNS query for the given domain/type/class."""
        # DNS header (12 bytes)
        header = struct.pack(
                "!HHHHHH",
                self.transaction_id,  # Transaction ID
                0x0100,               # Flags: standard query, RD=1
                1,                    # QDCOUNT: one question
                0,                    # ANCOUNT
                0,                    # NSCOUNT
                0,                    # ARCOUNT
                )

        # Question section
        labels = domain.split(".")
        question_data = b""
        for label in labels:
            if len(label) > 63:
                raise ValueError(f"Label too long: {label}")
            question_data += struct.pack("!B", len(label)) + label.encode("ascii")

        question_data += b"\x00"                 # terminating root label
        question_data += struct.pack("!HH", qtype, qclass)  # QTYPE + QCLASS

        return header + question_data

    def parse_dns_response(self, data: bytes, start_time: float, domain: str, qtype: int) -> DNSResponse:
        """Parse a DNS response into a DNSResponse model."""
        if len(data) < 12:
            raise ValueError("DNS response too short")

        response_time = time.time() - start_time
        id_, flags, qd, an, ns, ar = struct.unpack("!HHHHHH", data[:12])

        # Try to decode answer records via dnspython (best-effort)
        answer_records: List[str] = []
        try:
            msg = dns.message.from_wire(data)
            for rrset in msg.answer:
                for rr in rrset:
                    answer_records.append(str(rr))
        except Exception:
            # Ignore parse errors; they don't affect header-level checks
            pass

        return DNSResponse(
                rcode=flags & 0x0F,
                transaction_id=id_,
                flags=flags,
                questions=qd,
                answers=an,
                authority=ns,
                additional=ar,
                raw_data=data,
                response_time=response_time,
                answer_records=answer_records,
                domain_queried=domain,
                query_type=qtype,
                )

    # ------------------------------------------------------------------ #
    # Public query methods
    # ------------------------------------------------------------------ #

    def send_query(self, domain: str, qtype: int = 1) -> DNSResponse:
        """Send a single DNS query and return the parsed response."""
        self._log(f"Querying domain: {domain} (type={qtype})")
        query = self.create_dns_query(domain, qtype)

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(self.timeout)

        try:
            start_time = time.time()
            sock.sendto(query, (self.server_ip, self.server_port))
            response_data, _addr = sock.recvfrom(512)

            response = self.parse_dns_response(response_data, start_time, domain, qtype)
            self._log(
                    f"Response: rcode={response.rcode}, blocked={response.is_blocked()}, "
                    f"time={response.response_time:.3f}s"
                    )
            return response

        except socket.timeout:
            raise TimeoutError(f"DNS query timeout after {self.timeout}s for domain: {domain}")
        finally:
            sock.close()

    def send_malformed_query(self, malformed_data: bytes) -> Optional[DNSResponse]:
        """Send a raw, possibly malformed DNS datagram; return response or None on timeout."""
        self._log("Sending malformed query")
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(self.timeout)

        try:
            start_time = time.time()
            sock.sendto(malformed_data, (self.server_ip, self.server_port))
            response_data, _addr = sock.recvfrom(512)
            return self.parse_dns_response(response_data, start_time, "<malformed>", 1)

        except socket.timeout:
            self._log("No response to malformed query (expected)")
            return None
        finally:
            sock.close()

    async def send_concurrent_queries(self, domains: List[str], qtype: int = 1) -> List[Union[DNSResponse, Exception]]:
        """Send multiple DNS queries concurrently and gather results."""
        self._log(f"Sending {len(domains)} concurrent queries")

        async def send_single_query(domain: str) -> DNSResponse:
            loop = asyncio.get_event_loop()
            return await loop.run_in_executor(None, self.send_query, domain, qtype)

        tasks = [send_single_query(domain) for domain in domains]
        results = await asyncio.gather(*tasks, return_exceptions=True)
        self._log(f"Concurrent queries completed: {len(results)} results")
        return results

    # ------------------------------------------------------------------ #
    # Convenience verifiers
    # ------------------------------------------------------------------ #

    def verify_blocked(self, domain: str, qtype: int = 1) -> bool:
        """
        Verify a domain is blocked (expects REFUSED).

        Returns:
            True if blocked; False on error or unexpected rcode.
        """
        try:
            response = self.send_query(domain, qtype)
            return response.is_blocked()
        except Exception as e:
            self._log(f"Error verifying blocked domain {domain}: {e}")
            return False

    def verify_allowed(self, domain: str, qtype: int = 1) -> bool:
        """
        Verify a domain is allowed (expects NOERROR or NXDOMAIN).

        Returns:
            True if allowed; False on error or unexpected rcode.
        """
        try:
            response = self.send_query(domain, qtype)
            return response.is_allowed()
        except Exception as e:
            self._log(f"Error verifying allowed domain {domain}: {e}")
            return False

    # ------------------------------------------------------------------ #
    # Batch tests
    # ------------------------------------------------------------------ #

    def test_exact_domain_blocking(self, exact_domains: List[str]) -> BatchTestResult:
        """
        Test blocking for exact (non-wildcard) domains.

        Args:
            exact_domains: List of exact domains to test.
        """
        self._log(f"Testing {len(exact_domains)} exact domains for blocking")
        result = BatchTestResult(total=len(exact_domains), blocked=0, allowed=0, errors=0)

        for domain in exact_domains:
            try:
                response = self.send_query(domain)
                result.results.append((domain, response))
                if response.is_blocked():
                    result.blocked += 1
                elif response.is_allowed():
                    result.allowed += 1
            except Exception as e:
                self._log(f"Error testing domain {domain}: {e}")
                result.errors += 1

        self._log(f"Exact domain test complete: {result.blocked}/{result.total} blocked")
        return result

    def test_wildcard_blocking(self, wildcard_test_cases: List[str]) -> BatchTestResult:
        """
        Test blocking behavior for wildcard-matching cases.

        Args:
            wildcard_test_cases: List of domains expected to match wildcard rules.
        """
        self._log(f"Testing {len(wildcard_test_cases)} wildcard test cases")
        result = BatchTestResult(total=len(wildcard_test_cases), blocked=0, allowed=0, errors=0)

        for domain in wildcard_test_cases:
            try:
                response = self.send_query(domain)
                result.results.append((domain, response))
                if response.is_blocked():
                    result.blocked += 1
                elif response.is_allowed():
                    result.allowed += 1
            except Exception as e:
                self._log(f"Error testing wildcard domain {domain}: {e}")
                result.errors += 1

        self._log(f"Wildcard test complete: {result.blocked}/{result.total} blocked")
        return result

    def test_subdomain_blocking(self, parent_domain: str, subdomains: List[str]) -> BatchTestResult:
        """
        Test blocking for a list of subdomains of a given parent domain.

        Args:
            parent_domain: Parent domain (e.g., "example.com").
            subdomains: List of subdomain prefixes (e.g., ["www", "mail", "api"]).
        """
        full_domains = [f"{sub}.{parent_domain}" for sub in subdomains]
        self._log(f"Testing {len(full_domains)} subdomains of {parent_domain}")
        return self.test_exact_domain_blocking(full_domains)

    def test_allowed_domains(self, allowed_domains: List[str]) -> BatchTestResult:
        """
        Ensure legitimate domains are NOT blocked.

        Args:
            allowed_domains: List of domains that should be allowed.
        """
        self._log(f"Testing {len(allowed_domains)} domains should be allowed")
        result = BatchTestResult(total=len(allowed_domains), blocked=0, allowed=0, errors=0)

        for domain in allowed_domains:
            try:
                response = self.send_query(domain)
                result.results.append((domain, response))
                if response.is_blocked():
                    result.blocked += 1
                elif response.is_allowed():
                    result.allowed += 1
            except Exception as e:
                self._log(f"Error testing allowed domain {domain}: {e}")
                result.errors += 1

        self._log(f"Allowed domain test complete: {result.allowed}/{result.total} allowed")
        return result

    def test_categorized_domains(self, domain_categories: Dict[str, List[str]]) -> Dict[str, BatchTestResult]:
        """
        Run tests across multiple domain categories.

        Args:
            domain_categories: Mapping {category: [domains]}.
        """
        self._log(f"Testing {len(domain_categories)} domain categories")
        results: Dict[str, BatchTestResult] = {}
        for category, domains in domain_categories.items():
            self._log(f"Testing category: {category} ({len(domains)} domains)")
            results[category] = self.test_exact_domain_blocking(domains)
        return results

    async def test_concurrent_blocking(
            self,
            domains_should_block: List[str],
            domains_should_allow: List[str],
            ) -> Tuple[BatchTestResult, BatchTestResult]:
        """
        Concurrently test blocking and allowing behavior.

        Args:
            domains_should_block: Domains expected to be blocked.
            domains_should_allow: Domains expected to be allowed.

        Returns:
            (blocked_results, allowed_results)
        """
        self._log("Starting concurrent blocking/allowing test")

        all_domains = domains_should_block + domains_should_allow
        responses = await self.send_concurrent_queries(all_domains)

        blocked_result = BatchTestResult(total=len(domains_should_block))
        allowed_result = BatchTestResult(total=len(domains_should_allow))

        # Process results for the "should block" set
        for i, domain in enumerate(domains_should_block):
            response = responses[i]

            if isinstance(response, Exception):
                blocked_result.errors += 1
                blocked_result.failed_domains.append((domain, f"Exception: {type(response).__name__}"))
                continue

            if response is None:
                blocked_result.timeouts += 1
                blocked_result.failed_domains.append((domain, "Timeout (no response)"))
                continue

            blocked_result.results.append((domain, response))
            if response.is_blocked():
                blocked_result.blocked += 1
            else:
                blocked_result.allowed += 1
                blocked_result.failed_domains.append(
                        (domain, f"Expected REFUSED (rcode=5), got rcode={response.rcode}")
                        )

        # Process results for the "should allow" set
        for i, domain in enumerate(domains_should_allow):
            response = responses[len(domains_should_block) + i]

            if isinstance(response, Exception):
                allowed_result.errors += 1
                allowed_result.failed_domains.append((domain, f"Exception: {type(response).__name__}"))
                continue

            if response is None:
                allowed_result.timeouts += 1
                allowed_result.failed_domains.append((domain, "Timeout (no response)"))
                continue

            allowed_result.results.append((domain, response))
            if response.is_allowed():
                allowed_result.allowed += 1
            else:
                allowed_result.blocked += 1
                allowed_result.failed_domains.append(
                        (domain, f"Incorrectly blocked with rcode={response.rcode}")
                        )

        self._log(
                f"Concurrent test complete: {blocked_result.blocked}/{blocked_result.total} blocked, "
                f"{allowed_result.allowed}/{allowed_result.total} allowed"
                )
        return blocked_result, allowed_result

### end of file DnsClient.py ###
