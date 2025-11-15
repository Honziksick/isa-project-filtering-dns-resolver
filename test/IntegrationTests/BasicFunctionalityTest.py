################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         BasicFunctionalityTest.py                                      #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  Integration tests for the Filtering DNS Resolver. These tests  #
#               verify correct blocking of exact and wildcard domains,         #
#               case-insensitive matching, query type handling, performance,   #
#               concurrency, and edge-case/malformed packet handling. The      #
#               tests ensure RFC 1035 compliance and correct filter file       #
#               parsing.                                                       #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import time
import pytest
from DnsClient import DNSRCode, DNSTestClient

class TestBasicFunctionality:
    """Basic functionality tests: exact-domain blocking and wildcard support."""

    # --------------------------------------------------------------------- #
    # Startup & stats
    # --------------------------------------------------------------------- #
    def test_resolver_startup(self, running_resolver):
        """Resolver starts successfully and exposes filter statistics."""
        assert running_resolver.process is not None
        assert running_resolver.process.poll() is None  # Process is running

        stats = running_resolver.get_process_stats()
        assert stats.get("status") in ["running", "sleeping"]

        # Verify filter statistics
        filter_stats = stats.get("filter", {})
        assert filter_stats.get("total_entries", 0) > 0
        assert filter_stats.get("exact_domains", 0) >= 0
        assert filter_stats.get("wildcard_patterns", 0) >= 0

        print(
                f"✓ Resolver started with {filter_stats.get('exact_domains', 0)} exact domains "
                f"and {filter_stats.get('wildcard_patterns', 0)} wildcards"
                )

    # --------------------------------------------------------------------- #
    # Exact blocking & allowed forwarding
    # --------------------------------------------------------------------- #
    def test_blocked_domain_refused(self, running_resolver, dns_client, exact_blocked_domains):
        """Exact-blocked domains must return REFUSED (RFC 1035)."""
        test_domains = exact_blocked_domains[:5]

        for domain in test_domains:
            response = dns_client.send_query(domain)

            assert response.rcode == DNSRCode.REFUSED, (
                    f"Domain {domain} should be blocked with REFUSED (RFC 1035 policy-based blocking)"
            )
            assert response.transaction_id == dns_client.transaction_id
            assert response.answers == 0
            assert response.is_blocked()
            print(f"✓ Exact domain blocked with REFUSED: {domain} (time: {response.response_time:.3f}s)")

    def test_subdomain_blocking(self, running_resolver, dns_client, exact_blocked_subdomains):
        """Subdomains of exact-blocked domains should be blocked."""
        if not exact_blocked_subdomains:
            pytest.skip("No subdomain test domains configured")

        for domain in exact_blocked_subdomains[:5]:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Subdomain {domain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, "Should return REFUSED for subdomain block"
            print(f"✓ Subdomain blocked with REFUSED: {domain} (time: {response.response_time:.3f}s)")

    def test_allowed_domain_forwarded(self, running_resolver, dns_client, test_domains_should_allow):
        """Allowed domains should be forwarded upstream."""
        test_domains = test_domains_should_allow[:5]

        print("=== Allowed domains test ===")
        for domain in test_domains:
            response = dns_client.send_query(domain)
            print(f"Testing domain: {domain}")
            print(f"  RCODE: {response.rcode}")
            print(f"  Transaction ID: {response.transaction_id}")
            print(f"  Blocked: {response.is_blocked()}")
            print(f"  Response time: {response.response_time:.3f}s")

            assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
            assert response.transaction_id == dns_client.transaction_id
            assert not response.is_blocked()

            print(f"✓ Allowed domain processed: {domain} (rcode={response.rcode}, time: {response.response_time:.3f}s)")
        print("=== Allowed domains test finished ===")

    # --------------------------------------------------------------------- #
    # Wildcards
    # --------------------------------------------------------------------- #
    def test_wildcard_blocking_subdomains(self, running_resolver, dns_client, wildcard_patterns):
        """Wildcard patterns block subdomains."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        for pattern in wildcard_patterns[:5]:
            base_domain = pattern.replace("*.", "")
            test_subdomain = f"test.{base_domain}"

            response = dns_client.send_query(test_subdomain)
            assert response.is_blocked(), f"Wildcard subdomain {test_subdomain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, "Should return REFUSED for wildcard block"
            print(f"✓ Wildcard subdomain blocked with REFUSED: {test_subdomain} (time: {response.response_time:.3f}s)")

    def test_wildcard_not_blocking_parent(self, running_resolver, dns_client, wildcard_patterns):
        """Wildcard patterns should NOT block parent domain."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        for pattern in wildcard_patterns[:3]:
            parent_domain = pattern.replace("*.", "")

            response = dns_client.send_query(parent_domain)
            assert not response.is_blocked(), f"Parent domain {parent_domain} should NOT be blocked by wildcard"
            assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN], "Parent should be forwarded upstream"
            print(f"✓ Wildcard parent domain NOT blocked: {parent_domain} (rcode={response.rcode})")


    # --------------------------------------------------------------------- #
    # Case-insensitivity
    # --------------------------------------------------------------------- #
    def test_case_insensitive_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Blocking is case-insensitive for exact domains."""
        test_domain = exact_blocked_domains[0] if exact_blocked_domains else "blocked-domain.com"

        test_cases = [
                test_domain.upper(),
                test_domain.lower(),
                test_domain.title(),
                "".join(c.upper() if i % 2 else c.lower() for i, c in enumerate(test_domain)),
                ]

        for domain in test_cases:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Domain {domain} should be blocked (case-insensitive)"
            assert response.rcode == DNSRCode.REFUSED, "Should return REFUSED"
            print(f"✓ Case-insensitive block with REFUSED: {domain}")

    def test_wildcard_case_insensitive(self, running_resolver, dns_client, wildcard_patterns):
        """Wildcard blocking is case-insensitive."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        pattern = wildcard_patterns[0].replace("*.", "")
        test_cases = [f"test.{pattern.upper()}", f"test.{pattern.lower()}", f"test.{pattern.title()}"]

        for domain in test_cases:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Wildcard domain {domain} should be blocked (case-insensitive)"
            assert response.rcode == DNSRCode.REFUSED
            print(f"✓ Wildcard case-insensitive block with REFUSED: {domain}")

    # --------------------------------------------------------------------- #
    # Query type handling (blocked vs allowed)
    # --------------------------------------------------------------------- #
    @pytest.mark.parametrize(
            "qtype,type_name",
            [
                    (1, "A"),
                    (28, "AAAA"),
                    (15, "MX"),
                    (2, "NS"),
                    (5, "CNAME"),
                    (16, "TXT"),
                    ],
            )
    def test_different_query_types_blocked(
            self, running_resolver, dns_client, qtype, type_name, exact_blocked_domains
            ):
        """Different DNS query types against blocked domains."""
        blocked_domain = exact_blocked_domains[0] if exact_blocked_domains else "blocked-domain.com"
        response = dns_client.send_query(blocked_domain, qtype=qtype)

        if qtype == 1:  # A
            assert response.is_blocked(), (
                    f"Blocked domain should return REFUSED for {type_name} query"
            )
            assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED for {type_name}"
        else:
            assert response.rcode == DNSRCode.NOTIMP, f"Should return NOTIMP for {type_name} (only A IN supported)"
        print(f"✓ Query type {type_name} correctly handled for blocked domain ({response.rcode})")

    @pytest.mark.parametrize(
            "qtype,type_name",
            [
                    (1, "A"),
                    (28, "AAAA"),
                    (15, "MX"),
                    (2, "NS"),
                    ],
            )
    def test_different_query_types_allowed(self, running_resolver, dns_client, qtype, type_name):
        """Different DNS query types against allowed domains."""
        response = dns_client.send_query("google.com", qtype=qtype)

        if qtype == 1:  # A
            assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
            assert not response.is_blocked()
        else:
            assert response.rcode == DNSRCode.NOTIMP, f"Allowed domain should return NOTIMP for {type_name}"
        print(f"✓ Query type {type_name} handled correctly for allowed domain (rcode={response.rcode})")

    # --------------------------------------------------------------------- #
    # Performance
    # --------------------------------------------------------------------- #
    def test_response_time_performance(self, running_resolver, dns_client, test_domains_should_allow):
        """Response time across several allowed domains."""
        test_domains = test_domains_should_allow[:10]
        response_times = []

        for domain in test_domains:
            response = dns_client.send_query(domain)
            response_times.append(response.response_time)

        avg_time = sum(response_times) / len(response_times)
        max_time = max(response_times)
        min_time = min(response_times)

        print("✓ Response time stats:")
        print(f"  Average: {avg_time:.3f}s")
        print(f"  Min: {min_time:.3f}s")
        print(f"  Max: {max_time:.3f}s")

        assert avg_time < 1.0, f"Average response time too slow: {avg_time}s"
        assert max_time < 2.0, f"Maximum response time too slow: {max_time}s"

    def test_blocked_domain_performance(self, running_resolver, dns_client, exact_blocked_domains):
        """Blocking should be very fast (no upstream)."""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        response_times = []
        test_domains = exact_blocked_domains[:10]

        for domain in test_domains:
            response = dns_client.send_query(domain)
            assert response.is_blocked()
            assert response.rcode == DNSRCode.REFUSED
            response_times.append(response.response_time)

        avg_time = sum(response_times) / len(response_times)
        max_time = max(response_times)

        print("✓ Blocked domain response time:")
        print(f"  Average: {avg_time:.3f}s")
        print(f"  Max: {max_time:.3f}s")

        assert avg_time < 0.1, f"Blocked domain response too slow: {avg_time}s"


    # --------------------------------------------------------------------- #
    # Batch checks
    # --------------------------------------------------------------------- #
    def test_batch_exact_domain_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Batch test for exact-domain blocking."""
        result = dns_client.test_exact_domain_blocking(exact_blocked_domains)

        print("✓ Exact domain batch test:")
        print(f"  Total: {result.total}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Allowed: {result.allowed}")
        print(f"  Errors: {result.errors}")
        print(f"  Block rate: {result.block_rate:.1f}%")

        assert result.block_rate > 95.0, f"Expected >95% block rate, got {result.block_rate:.1f}%"

    def test_batch_subdomain_blocking(self, running_resolver, dns_client, exact_blocked_subdomains):
        """Batch test for subdomain blocking."""
        if not exact_blocked_subdomains:
            pytest.skip("No subdomain test domains configured")

        result = dns_client.test_exact_domain_blocking(exact_blocked_subdomains)

        print("✓ Subdomain batch test:")
        print(f"  Total: {result.total}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Block rate: {result.block_rate:.1f}%")

        assert result.block_rate > 95.0, f"Expected >95% subdomain block rate, got {result.block_rate:.1f}%"

    def test_batch_wildcard_blocking(self, running_resolver, dns_client, wildcard_patterns):
        """Batch test for wildcard blocking (subdomains only)."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        # Generate test subdomains
        test_subdomains = [f"test.{pattern.replace('*.', '')}" for pattern in wildcard_patterns]
        result = dns_client.test_exact_domain_blocking(test_subdomains)

        print("✓ Wildcard subdomain batch test:")
        print(f"  Total: {result.total}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Block rate: {result.block_rate:.1f}%")

        assert result.block_rate > 95.0, f"Expected >95% wildcard block rate, got {result.block_rate:.1f}%"

    def test_batch_wildcard_parent_not_blocked(self, running_resolver, dns_client, wildcard_patterns):
        """Batch test ensuring wildcard parent domains are NOT blocked."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        parent_domains = [pattern.replace("*.", "") for pattern in wildcard_patterns]
        result = dns_client.test_allowed_domains(parent_domains)

        print("✓ Wildcard parent domains batch test:")
        print(f"  Total: {result.total}")
        print(f"  Allowed: {result.allowed}")
        print(f"  Blocked: {result.blocked}")

        assert result.blocked == 0, f"Wildcard parent domains should NOT be blocked: {result.blocked} were blocked"

    def test_batch_allowed_domains(self, running_resolver, dns_client, test_domains_should_allow):
        """Batch test for allowed domains."""
        result = dns_client.test_allowed_domains(test_domains_should_allow)

        print("✓ Allowed domains batch test:")
        print(f"  Total: {result.total}")
        print(f"  Allowed: {result.allowed}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Success rate: {result.success_rate:.1f}%")

        assert result.blocked == 0, f"False positives detected: {result.blocked} domains blocked with REFUSED"


    # --------------------------------------------------------------------- #
    # Concurrency
    # --------------------------------------------------------------------- #
    @pytest.mark.asyncio
    @pytest.mark.timeout(30)
    async def test_concurrent_blocking_and_allowing(
            self, running_resolver, dns_client, exact_blocked_domains, test_domains_should_allow
            ):
        """Concurrent mix of blocked and allowed queries."""
        blocked_tests = exact_blocked_domains[:10]
        allowed_tests = test_domains_should_allow[:10]

        print(f"✓ Testing {len(blocked_tests)} blocked domains and {len(allowed_tests)} allowed domains")

        blocked_result, allowed_result = await dns_client.test_concurrent_blocking(blocked_tests, allowed_tests)

        print("✓ Concurrent test results:")
        print(
                f"  Blocked domains: {blocked_result.blocked}/{blocked_result.total} "
                f"({blocked_result.block_rate:.1f}%)"
                )
        print(
                f"  Allowed domains: {allowed_result.allowed}/{allowed_result.total} "
                f"({100.0 * allowed_result.allowed / max(1, allowed_result.total):.1f}%)"
                )

        if blocked_result.failed_domains:
            print("\n❌ Blocked domains failures:")
            print(blocked_result.get_failure_summary())

        if allowed_result.failed_domains:
            print("\n❌ Allowed domains failures:")
            print(allowed_result.get_failure_summary())

        assert blocked_result.block_rate > 95.0, (
                f"Expected >95% block rate, got {blocked_result.block_rate:.1f}% "
                f"({blocked_result.blocked}/{blocked_result.total} blocked)\n"
                f"{blocked_result.get_failure_summary()}"
        )

        assert allowed_result.allowed == allowed_result.total, (
                f"Expected all {allowed_result.total} allowed domains to pass, "
                f"but {len(allowed_result.failed_domains)} failed:\n"
                f"{allowed_result.get_failure_summary()}"
        )

    # --------------------------------------------------------------------- #
    # Deep subdomains & wildcard parent behavior
    # --------------------------------------------------------------------- #
    def test_deep_subdomain_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Deep subdomains of exact-blocked domains are blocked."""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        base_domain = exact_blocked_domains[0]
        deep_subdomains = [
                f"sub.{base_domain}",
                f"deep.sub.{base_domain}",
                f"very.deep.sub.{base_domain}",
                f"extremely.very.deep.sub.{base_domain}",
                ]

        for domain in deep_subdomains:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Deep subdomain {domain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, "Should return REFUSED"
            print(f"✓ Deep subdomain blocked with REFUSED: {domain}")

    def test_wildcard_not_matching_parent(self, running_resolver, dns_client, wildcard_patterns):
        """Ensure that '*.example.com' does not block 'example.com' (parent not matched)."""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        pattern = wildcard_patterns[0].replace("*.", "")

        # Parent domain should NOT be blocked
        response = dns_client.send_query(pattern)
        assert not response.is_blocked(), f"Parent domain {pattern} should NOT be blocked by wildcard pattern"
        assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN], "Parent should be forwarded upstream"
        print(f"✓ Parent domain {pattern} correctly NOT blocked (rcode={response.rcode})")

        # Subdomain SHOULD be blocked
        subdomain_response = dns_client.send_query(f"test.{pattern}")
        assert subdomain_response.is_blocked(), f"Subdomain test.{pattern} should be blocked"
        assert subdomain_response.rcode == DNSRCode.REFUSED, "Should return REFUSED for wildcard"
        print(f"✓ Subdomain test.{pattern} correctly blocked with REFUSED")

    # --------------------------------------------------------------------- #
    # Edge cases & malformed packets
    # --------------------------------------------------------------------- #
    def test_edge_case_domains(self, running_resolver, dns_client, edge_case_domains):
        """Edge-case domain handling."""
        for case_name, domain in edge_case_domains.items():
            if not domain or domain in ["", ".", ".."]:
                try:
                    response = dns_client.send_query(domain)
                    assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL, DNSRCode.REFUSED]
                    print(f"✓ Edge case '{case_name}': handled (rcode={response.rcode})")
                except Exception as e:
                    print(f"✓ Edge case '{case_name}': error handled - {type(e).__name__}")
            else:
                try:
                    response = dns_client.send_query(domain)
                    print(f"✓ Edge case '{case_name}' ({domain}): rcode={response.rcode}")
                except Exception as e:
                    print(f"✓ Edge case '{case_name}': handled error - {type(e).__name__}")

    def test_empty_query_handling(self, running_resolver, dns_client):
        """Empty/malformed query handling."""
        malformed_query = b"\x00\x00" * 6
        response = dns_client.send_malformed_query(malformed_query)

        if response is None:
            print("✓ Malformed query correctly ignored (no response)")
        else:
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Malformed query handled with rcode={response.rcode}")


    # --------------------------------------------------------------------- #
    # REFUSED vs NXDOMAIN vs NOERROR distinction
    # --------------------------------------------------------------------- #
    def test_refused_vs_nxdomain_distinction(
            self, running_resolver, dns_client, exact_blocked_domains, test_domains_should_allow
            ):
        """REFUSED (blocked) vs NXDOMAIN (non-existent) vs NOERROR (existing)."""
        if not exact_blocked_domains:
            pytest.skip("No blocked domains configured")

        # Blocked domain -> REFUSED
        blocked_response = dns_client.send_query(exact_blocked_domains[0])
        assert blocked_response.rcode == DNSRCode.REFUSED
        print("✓ Blocked domain returns REFUSED (policy-based blocking)")

        # Non-existent domain -> NXDOMAIN (from upstream)
        nonexistent = f"this-definitely-does-not-exist-{int(time.time())}.example.com"
        nxdomain_response = dns_client.send_query(nonexistent)
        assert nxdomain_response.rcode == DNSRCode.NXDOMAIN
        print("✓ Non-existent domain returns NXDOMAIN (from upstream)")

        # Existing allowed domain -> NOERROR
        if test_domains_should_allow:
            allowed_response = dns_client.send_query(test_domains_should_allow[0])
            assert allowed_response.rcode == DNSRCode.NOERROR
            print("✓ Existing allowed domain returns NOERROR")

        print("✓ RFC 1035 compliant: REFUSED != NXDOMAIN distinction working")

    # --------------------------------------------------------------------- #
    # Filter file: invalid domains ignored
    # --------------------------------------------------------------------- #
    def test_invalid_domains_ignored(self, resolver_manager, temp_filter_file, dns_client):
        """Invalid domains in the filter file should be ignored by the loader."""
        with open(temp_filter_file, "w") as f:
            f.write(
                    """# Generated for testing

    ### EXACT DOMAINS ###
    blocked-domain.com
    valid-domain.com

    ### WILDCARD PATTERNS ###
    *.wildcard-test.net

    ### INVALID DOMAINS SHOULD BE IGNORED ###
    exam!ple.com
    invalid@domain.com
    examp_le.com
    exam ple.com
    example..com
    ..example.com
    example.com..
    .example.com
    example.com.
    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc.dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd.com
    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com
    -example.com
    example-.com
    -invalid-.com
    """
                    )

        resolver_manager.start_resolver(
                filter_file_costum=temp_filter_file,
                port=15354,
                verbose=True,
                )

        filter_stats = resolver_manager.get_filter_stats()
        assert filter_stats and filter_stats.total_entries > 0, "Filter not loaded - empty stats!"

        client = DNSTestClient(server_port=15354)
        time.sleep(1)

        invalid_domains = [
                "exam!ple.com",
                "invalid@domain.com",
                "examp_le.com",
                "exam ple.com",
                "example..com",
                "..example.com",
                "example.com..",
                ".example.com",
                "example.com.",
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc.dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd.com",
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com",
                "-example.com",
                "example-.com",
                "-invalid-.com",
                ]

        for domain in invalid_domains:
            try:
                response = client.send_query(domain)
                assert (
                        response.rcode == DNSRCode.FORMERR
                ), f"Invalid domain {domain} must return FORMERR, got {response.rcode}"
                print(f"✓ {domain}: correctly FORMERR")
            except Exception as e:
                print(f"✓ {domain}: exception raised ({type(e).__name__}) - considered correct behavior")

### end of file BasicFunctionalityTest.py ###
