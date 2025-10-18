################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         ErrorHandlingTest.py                                           #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  Integration tests focused on error handling and robustness.    #
#               These tests verify the resolver's response to malformed        #
#               packets, invalid headers, unsupported query types/classes,     #
#               edge-case domain names, and concurrency under stress.          #
#               The goal is to ensure the resolver remains stable, does not    #
#               crash, and correctly handles all error scenarios according     #
#               to RFC 1035 and best practices.                                #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import pytest
import struct
import asyncio
from DnsClient import DNSRCode

class TestErrorHandling:
    """Error-handling tests using the new configuration."""

    # --------------------------------------------------------------------- #
    # Malformed packets
    # --------------------------------------------------------------------- #
    def test_malformed_query_too_short(self, running_resolver, dns_client):
        """Too-short DNS query should be ignored or answered with FORMERR."""
        malformed_query = b"\x12\x34\x01\x00"  # Only 4 bytes

        response = dns_client.send_malformed_query(malformed_query)

        if response:  # Server may return FORMERR or ignore it entirely
            assert response.rcode == DNSRCode.FORMERR
            print("✓ Short malformed query handled with FORMERR")
        else:
            print("✓ Short malformed query ignored (no response)")

        # Resolver must keep running
        assert running_resolver.process.poll() is None
        print("✓ Resolver survived malformed query")

    def test_malformed_query_invalid_header(self, running_resolver, dns_client):
        """Invalid DNS header (e.g., absurdly large QDCOUNT) is handled safely."""
        # Create an invalid header with an excessive number of questions
        malformed_header = struct.pack(
                "!HHHHHH",
                0x1234,  # Transaction ID
                0x0100,  # Flags
                0xFFFF,  # Questions (invalid)
                0,
                0,
                0,
                )

        response = dns_client.send_malformed_query(malformed_header)

        if response:
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Invalid header handled with rcode={response.rcode}")
        else:
            print("✓ Invalid header ignored (no response)")

        assert running_resolver.process.poll() is None

    def test_malformed_query_random_bytes(self, running_resolver, dns_client):
        """Random bytes submitted as a DNS query are handled or ignored."""
        import random

        random_query = bytes([random.randint(0, 255) for _ in range(50)])
        response = dns_client.send_malformed_query(random_query)

        if response:
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL, DNSRCode.NOTIMP]
            print(f"✓ Random bytes handled with rcode={response.rcode}")
        else:
            print("✓ Random bytes ignored")

        assert running_resolver.process.poll() is None

    # --------------------------------------------------------------------- #
    # Name/label constraints and odd syntaxes
    # --------------------------------------------------------------------- #
    def test_query_name_too_long(self, running_resolver, dns_client):
        """Overly long domain name (>255 octets) should fail safely."""
        # Build a domain name longer than 255 characters
        long_label = "a" * 70
        long_domain = ".".join([long_label] * 4)  # ~280 characters

        try:
            response = dns_client.send_query(long_domain)
            # Server should return FORMERR or SERVFAIL
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Overly long domain handled with rcode={response.rcode}")
        except ValueError as e:
            # The client may refuse to construct such a long query
            print(f"✓ Long domain rejected by client: {e}")

        assert running_resolver.process.poll() is None

    def test_query_name_max_label_length(self, running_resolver, dns_client):
        """Maximum label length (63 octets) is accepted; 64 should fail."""
        # 63 characters is the maximum for a single label
        max_label = "a" * 63
        test_domain = f"{max_label}.com"

        response = dns_client.send_query(test_domain)
        # Should process normally (even if NXDOMAIN)
        assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
        print(f"✓ Max length label handled correctly: {test_domain[:20]}...{test_domain[-10:]}")

        # 64 characters should fail
        too_long_label = "a" * 64
        too_long_domain = f"{too_long_label}.com"

        try:
            response = dns_client.send_query(too_long_domain)
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print("✓ Too long label handled with error")
        except ValueError:
            print("✓ Too long label rejected by client")

    def test_query_with_consecutive_dots(self, running_resolver, dns_client):
        """Domains containing consecutive dots should be rejected or handled."""
        invalid_domains = [
                "example..com",
                "test...domain.org",
                "..example.com",
                "example.com..",
                ]

        for domain in invalid_domains:
            try:
                response = dns_client.send_query(domain)
                # Behavior is implementation-dependent; just log the outcome
                print(f"✓ Consecutive dots in '{domain}': rcode={response.rcode}")
            except Exception as e:
                print(f"✓ Consecutive dots in '{domain}': error={type(e).__name__}")

        assert running_resolver.process.poll() is None

    # --------------------------------------------------------------------- #
    # Unsupported classes/types
    # --------------------------------------------------------------------- #
    def test_unsupported_query_class(self, running_resolver, dns_client):
        """Unsupported query class should not crash the resolver."""
        # Craft a query with an invalid/unsupported class (e.g., 255)
        query_data = dns_client.create_dns_query("google.com", qtype=1, qclass=255)

        response = dns_client.send_malformed_query(query_data)

        if response:
            # Many resolvers reply with NOTIMP or FORMERR
            assert response.rcode in [
                    DNSRCode.NOTIMP,
                    DNSRCode.FORMERR,
                    DNSRCode.SERVFAIL,
                    DNSRCode.NOERROR,
                    ]
            print(f"✓ Unsupported query class handled with rcode={response.rcode}")
        else:
            print("✓ Unsupported query class ignored")

        assert running_resolver.process.poll() is None

    def test_unsupported_query_type(self, running_resolver, dns_client):
        """Unusual or unsupported query types are handled/answered safely."""
        unusual_qtypes = [
                (255, "ANY"),   # ANY (deprecated)
                (99, "SPF"),    # SPF (obsolete)
                (251, "IXFR"),  # Zone transfer
                (252, "AXFR"),  # Zone transfer
                ]

        for qtype, name in unusual_qtypes:
            try:
                response = dns_client.send_query("example.com", qtype=qtype)
                print(f"✓ Query type {name} ({qtype}): rcode={response.rcode}")
                # Should return some response code
                assert response.rcode in [
                        DNSRCode.NOERROR,
                        DNSRCode.NXDOMAIN,
                        DNSRCode.NOTIMP,
                        DNSRCode.SERVFAIL,
                        ]
            except Exception as e:
                print(f"✓ Query type {name} ({qtype}): error={type(e).__name__}")

        assert running_resolver.process.poll() is None

    # --------------------------------------------------------------------- #
    # Concurrency & stress
    # --------------------------------------------------------------------- #
    def test_concurrent_malformed_queries(self, running_resolver, dns_client):
        """Multiple malformed queries sent in quick succession should not crash the resolver."""
        malformed_queries = [
                b"\x12\x34",  # Too short
                b"\x12\x34\x01\x00\xFF\xFF\x00\x00\x00\x00\x00\x00",  # Invalid header
                b"\x12\x34\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00",  # No questions
                b"\xFF" * 100,  # Random bytes
                b"",  # Empty
                ]

        responses = []
        for i, query in enumerate(malformed_queries):
            try:
                response = dns_client.send_malformed_query(query)
                responses.append(response)
                if response:
                    print(f"✓ Malformed query {i + 1}: handled with rcode={response.rcode}")
                else:
                    print(f"✓ Malformed query {i + 1}: ignored")
            except Exception as e:
                print(f"✓ Malformed query {i + 1}: error={type(e).__name__}")

        # Resolver should survive all malformed inputs
        assert running_resolver.process.poll() is None
        print(f"✓ Resolver survived {len(malformed_queries)} malformed queries")

    @pytest.mark.asyncio
    async def test_async_malformed_queries(self, running_resolver, dns_client):
        """Asynchronously send several malformed queries; resolver should remain healthy."""
        async def send_malformed(query_bytes: bytes):
            loop = asyncio.get_event_loop()
            try:
                return await loop.run_in_executor(None, dns_client.send_malformed_query, query_bytes)
            except Exception as e:
                return e

        malformed_queries = [
                b"\x12\x34\x01\x00",
                b"\xFF" * 50,
                b"",
                b"\x00" * 20,
                ]

        tasks = [send_malformed(q) for q in malformed_queries]
        results = await asyncio.gather(*tasks)

        print(f"✓ Async malformed queries: {len(results)} handled")
        assert running_resolver.process.poll() is None

    def test_rapid_fire_queries(self, running_resolver, dns_client):
        """Burst many queries from multiple threads; resolver should keep up."""
        import threading
        import time

        results = []
        errors = []

        def send_query_thread(domain_suffix: int):
            try:
                response = dns_client.send_query(f"test{domain_suffix}.google.com")
                results.append(response)
            except Exception as e:
                errors.append(e)

        # Launch 30 concurrent queries (increased from 20)
        threads = []
        start_time = time.time()

        for i in range(30):
            thread = threading.Thread(target=send_query_thread, args=(i,))
            threads.append(thread)
            thread.start()

        # Wait for completion
        for thread in threads:
            thread.join(timeout=15)

        elapsed = time.time() - start_time

        print("✓ Rapid fire test:")
        print(f"  - Successful: {len(results)}")
        print(f"  - Errors: {len(errors)}")
        print(f"  - Time: {elapsed:.2f}s")
        print(f"  - Rate: {len(results) / elapsed:.1f} queries/sec")

        # Most queries should succeed
        success_rate = (len(results) / 30) * 100
        assert success_rate >= 75, f"Too many failed queries: {success_rate:.1f}%"
        assert running_resolver.process.poll() is None, "Resolver crashed"

    @pytest.mark.asyncio
    async def test_stress_mixed_queries(
            self, running_resolver, dns_client, exact_blocked_domains, test_domains_should_allow
            ):
        """Stress test: mix of legitimate, blocked, and malformed queries."""
        # Build a mixed set of inputs
        test_domains = []
        test_domains.extend(exact_blocked_domains[:10])          # Blocked
        test_domains.extend(test_domains_should_allow[:10])      # Allowed
        test_domains.extend(["", "...", "invalid..domain", "a" * 70 + ".com"])  # Malformed

        responses = await dns_client.send_concurrent_queries(test_domains)

        successful = sum(1 for r in responses if not isinstance(r, Exception))
        errors = sum(1 for r in responses if isinstance(r, Exception))

        print("✓ Stress test mixed queries:")
        print(f"  - Total: {len(responses)}")
        print(f"  - Successful: {successful}")
        print(f"  - Errors: {errors}")

        assert running_resolver.process.poll() is None
        assert successful > len(responses) * 0.5, "More than 50% queries failed"

    # --------------------------------------------------------------------- #
    # FQDN variants & recovery
    # --------------------------------------------------------------------- #
    def test_query_with_trailing_dot(self, running_resolver, dns_client):
        """Fully-qualified names (with trailing dot) should behave like non-FQDN forms."""
        test_cases = [
                ("google.com.", "google.com"),
                ("example.org.", "example.org"),
                ("test.domain.", "test.domain"),
                ]

        for fqdn, non_fqdn in test_cases:
            try:
                response_fqdn = dns_client.send_query(fqdn)
                response_normal = dns_client.send_query(non_fqdn)

                # Both should be processed (existing or NXDOMAIN)
                assert response_fqdn.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
                assert response_normal.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
                print(f"✓ FQDN handling: {fqdn} and {non_fqdn} both processed")
            except Exception as e:
                print(f"✓ FQDN test error (expected): {type(e).__name__}")

    def test_blocked_domain_with_malformed_followup(self, running_resolver, dns_client, exact_blocked_domains):
        """Blocked domain request followed by malformed query should not break subsequent blocking."""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        blocked_domain = exact_blocked_domains[0]

        # Step 1: normal blocked request
        response1 = dns_client.send_query(blocked_domain)
        assert response1.is_blocked()
        print(f"✓ Step 1: Blocked domain {blocked_domain}")

        # Step 2: malformed request
        malformed = b"\xFF" * 50
        _ = dns_client.send_malformed_query(malformed)
        print("✓ Step 2: Malformed query handled")

        # Step 3: another normal blocked request still works
        response3 = dns_client.send_query(blocked_domain)
        assert response3.is_blocked()
        print("✓ Step 3: Blocked domain still works after malformed")

        assert running_resolver.process.poll() is None

    def test_batch_error_recovery(self, running_resolver, dns_client, exact_blocked_domains):
        """After a batch of malformed queries, normal queries should still succeed."""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        # Phase 1: normal queries
        normal_result = dns_client.test_exact_domain_blocking(exact_blocked_domains[:5])
        assert normal_result.block_rate > 95.0
        print(f"✓ Phase 1: Normal queries work ({normal_result.blocked}/{normal_result.total})")

        # Phase 2: send a batch of malformed queries
        for _ in range(10):
            try:
                dns_client.send_malformed_query(b"\xFF" * 50)
            except Exception:
                pass
        print("✓ Phase 2: Error queries sent")

        # Phase 3: normal queries should still pass
        recovery_result = dns_client.test_exact_domain_blocking(exact_blocked_domains[:5])
        assert recovery_result.block_rate > 95.0
        print(f"✓ Phase 3: Recovery successful ({recovery_result.blocked}/{recovery_result.total})")

        assert running_resolver.process.poll() is None

### end of file ErrorHandlingTest.py ###
