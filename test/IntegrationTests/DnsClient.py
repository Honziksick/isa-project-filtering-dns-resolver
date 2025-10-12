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


class DNSRCode(IntEnum):
    """DNS Response Codes"""
    NOERROR = 0
    FORMERR = 1
    SERVFAIL = 2
    NXDOMAIN = 3
    NOTIMP = 4
    REFUSED = 5


@dataclass
class DNSResponse:
    """DNS odpověď s parsovanými daty"""
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
        """Kontroluje, zda byla doména zablokována (REFUSED)"""
        return self.rcode == DNSRCode.REFUSED

    def is_allowed(self) -> bool:
        """Kontroluje, zda byla doména povolena (NOERROR)"""
        return self.rcode in (DNSRCode.NOERROR, DNSRCode.NXDOMAIN)

    def has_answers(self) -> bool:
        """Kontroluje, zda odpověď obsahuje záznamy"""
        return self.answers > 0 and len(self.answer_records) > 0


@dataclass
class BatchTestResult:
    """Výsledek batch testování"""
    total: int = 0
    blocked: int = 0
    allowed: int = 0
    errors: int = 0
    timeouts: int = 0
    results: List[Tuple[str, DNSResponse]] = field(default_factory=list)
    failed_domains: List[Tuple[str, str]] = field(default_factory=list)

    @property
    def success_rate(self) -> float:
        """Úspěšnost testů (bez chyb)"""
        if self.total == 0:
            return 0.0
        return ((self.total - self.errors) / self.total) * 100.0

    @property
    def block_rate(self) -> float:
        """Procento zablokovaných"""
        if self.total == 0:
            return 0.0
        return (self.blocked / self.total) * 100.0

    def get_failure_summary(self) -> str:
        """Vrátí přehledný seznam selhavších domén"""
        if not self.failed_domains:
            return "No failures"

        lines = ["Failed domains:"]
        for domain, reason in self.failed_domains:
            lines.append(f"  - {domain}: {reason}")
        return "\n".join(lines)



class DNSTestClient:
    """Klient pro testování DNS resolveru"""

    def __init__(
            self,
            server_ip: str = "127.0.0.1",
            server_port: int = 15353,
            timeout: float = 10.0,
            verbose: bool = False):

        self.server_ip = server_ip
        self.server_port = server_port
        self.timeout = timeout
        self.verbose = verbose
        self.transaction_id = 0x1234

    def _log(self, message: str):
        """Logování pokud je verbose zapnutý"""
        if self.verbose:
            print(f"[DNSTestClient] {message}")

    def create_dns_query(self, domain: str, qtype: int = 1, qclass: int = 1) -> bytes:
        """Vytvoří raw DNS dotaz"""
        # DNS Header (12 bytes)
        header = struct.pack('!HHHHHH',
                             self.transaction_id,  # Transaction ID
                             0x0100,              # Flags: Standard query, RD=1
                             1,                   # Questions
                             0,                   # Answers
                             0,                   # Authority
                             0)                   # Additional

        # Question section
        labels = domain.split('.')
        question_data = b''

        for label in labels:
            if len(label) > 63:
                raise ValueError(f"Label too long: {label}")
            question_data += struct.pack('!B', len(label)) + label.encode('ascii')

        question_data += b'\x00'  # Root label
        question_data += struct.pack('!HH', qtype, qclass)

        return header + question_data

    def parse_dns_response(self, data: bytes, start_time: float, domain: str, qtype: int) -> DNSResponse:
        """Parsuje DNS odpověď"""
        if len(data) < 12:
            raise ValueError("DNS response too short")

        response_time = time.time() - start_time
        header = struct.unpack('!HHHHHH', data[:12])

        # Parsování answer records pomocí dnspython
        answer_records = []
        try:
            msg = dns.message.from_wire(data)
            for rrset in msg.answer:
                for rr in rrset:
                    answer_records.append(str(rr))
        except Exception:
            pass  # Ignore parsing errors for answer records

        return DNSResponse(
                rcode=header[1] & 0x0F,
                transaction_id=header[0],
                flags=header[1],
                questions=header[2],
                answers=header[3],
                authority=header[4],
                additional=header[5],
                raw_data=data,
                response_time=response_time,
                answer_records=answer_records,
                domain_queried=domain,
                query_type=qtype
                )

    def send_query(self, domain: str, qtype: int = 1) -> DNSResponse:
        """Pošle DNS dotaz a vrátí odpověď"""
        self._log(f"Querying domain: {domain} (type={qtype})")
        query = self.create_dns_query(domain, qtype)

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(self.timeout)

        try:
            start_time = time.time()
            sock.sendto(query, (self.server_ip, self.server_port))
            response_data, addr = sock.recvfrom(512)

            response = self.parse_dns_response(response_data, start_time, domain, qtype)
            self._log(f"Response: rcode={response.rcode}, blocked={response.is_blocked()}, "
                      f"time={response.response_time:.3f}s")
            return response

        except socket.timeout:
            raise TimeoutError(f"DNS query timeout after {self.timeout}s for domain: {domain}")
        finally:
            sock.close()

    def send_malformed_query(self, malformed_data: bytes) -> Optional[DNSResponse]:
        """Pošle neplatný DNS dotaz"""
        self._log("Sending malformed query")
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(self.timeout)

        try:
            start_time = time.time()
            sock.sendto(malformed_data, (self.server_ip, self.server_port))
            response_data, addr = sock.recvfrom(512)

            return self.parse_dns_response(response_data, start_time, "<malformed>", 1)

        except socket.timeout:
            self._log("No response to malformed query (expected)")
            return None
        except Exception as e:
            raise e
        finally:
            sock.close()

    async def send_concurrent_queries(self, domains: List[str], qtype: int = 1) -> List[Union[DNSResponse, Exception]]:
        """Pošle souběžné DNS dotazy"""
        self._log(f"Sending {len(domains)} concurrent queries")

        async def send_single_query(domain: str) -> DNSResponse:
            loop = asyncio.get_event_loop()
            return await loop.run_in_executor(None, self.send_query, domain, qtype)

        tasks = [send_single_query(domain) for domain in domains]
        results = await asyncio.gather(*tasks, return_exceptions=True)
        self._log(f"Concurrent queries completed: {len(results)} results")
        return results

    # Nové metody pro práci s configem

    def verify_blocked(self, domain: str, qtype: int = 1) -> bool:
        """
        Ověří, že doména je zablokována (vrací NXDOMAIN).

        Args:
            domain: Doména k ověření
            qtype: Typ DNS záznamu

        Returns:
            True pokud je doména zablokována, False pokud ne
        """
        try:
            response = self.send_query(domain, qtype)
            return response.is_blocked()
        except Exception as e:
            self._log(f"Error verifying blocked domain {domain}: {e}")
            return False

    def verify_allowed(self, domain: str, qtype: int = 1) -> bool:
        """
        Ověří, že doména je povolena (vrací NOERROR).

        Args:
            domain: Doména k ověření
            qtype: Typ DNS záznamu

        Returns:
            True pokud je doména povolena, False pokud ne
        """
        try:
            response = self.send_query(domain, qtype)
            return response.is_allowed()
        except Exception as e:
            self._log(f"Error verifying allowed domain {domain}: {e}")
            return False

    def test_exact_domain_blocking(self, exact_domains: List[str]) -> BatchTestResult:
        """
        Otestuje blokování exact domén.

        Args:
            exact_domains: Seznam exact domén k testování

        Returns:
            BatchTestResult s výsledky
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
        Otestuje blokování wildcard vzorů.

        Args:
            wildcard_test_cases: Seznam domén které by měly matchovat wildcardy

        Returns:
            BatchTestResult s výsledky
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
        Otestuje blokování subdomén pro danou parent doménu.

        Args:
            parent_domain: Rodičovská doména (např. "example.com")
            subdomains: Seznam subdomain prefixů (např. ["www", "mail", "api"])

        Returns:
            BatchTestResult s výsledky
        """
        full_domains = [f"{sub}.{parent_domain}" for sub in subdomains]
        self._log(f"Testing {len(full_domains)} subdomains of {parent_domain}")
        return self.test_exact_domain_blocking(full_domains)

    def test_allowed_domains(self, allowed_domains: List[str]) -> BatchTestResult:
        """
        Otestuje, že legitimní domény NEJSOU blokovány.

        Args:
            allowed_domains: Seznam domén které by měly být povoleny

        Returns:
            BatchTestResult s výsledky
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
        Otestuje domény rozdělené do kategorií.

        Args:
            domain_categories: Slovník {kategorie: [seznam domén]}

        Returns:
            Slovník {kategorie: BatchTestResult}
        """
        self._log(f"Testing {len(domain_categories)} domain categories")
        results = {}

        for category, domains in domain_categories.items():
            self._log(f"Testing category: {category} ({len(domains)} domains)")
            results[category] = self.test_exact_domain_blocking(domains)

        return results

    async def test_concurrent_blocking(
            self,
            domains_should_block: List[str],
            domains_should_allow: List[str]
            ) -> Tuple[BatchTestResult, BatchTestResult]:
        """
        Otestuje blokování a povolení domén souběžně.

        Args:
            domains_should_block: Domény které by měly být zablokovány
            domains_should_allow: Domény které by měly být povoleny

        Returns:
            Tuple (výsledky blokovaných, výsledky povolených)
        """
        self._log("Starting concurrent blocking/allowing test")

        all_domains = domains_should_block + domains_should_allow
        responses = await self.send_concurrent_queries(all_domains)

        blocked_result = BatchTestResult(total=len(domains_should_block))
        allowed_result = BatchTestResult(total=len(domains_should_allow))

        # Zpracování výsledků pro blokované domény
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

        # Zpracování výsledků pro povolené domény
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

        self._log(f"Concurrent test complete: {blocked_result.blocked}/{blocked_result.total} blocked, "
                  f"{allowed_result.allowed}/{allowed_result.total} allowed")

        return blocked_result, allowed_result

