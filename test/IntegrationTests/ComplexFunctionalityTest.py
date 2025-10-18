################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         ComplexFunctionalityTest.py                                    #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  Advanced integration and stress tests for the Filtering DNS    #
#               Resolver. These tests cover concurrency, burst and sustained   #
#               load, filter file edge cases, batch categorization, and mixed  #
#               query types. The goal is to verify resolver stability,         #
#               performance, and correct blocking/allowing of domains under    #
#               complex and high-load scenarios, including RFC 1035            #
#               compliance.                                                    #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import pytest
import asyncio
import time
import random
from DnsClient import DNSRCode, DNSTestClient

class TestComplexFunctionality:
    """Comprehensive test scenarios using the new configuration."""

    # --------------------------------------------------------------------- #
    # Concurrent mixed queries (exact / wildcard / allowed)
    # --------------------------------------------------------------------- #
    @pytest.mark.asyncio
    async def test_concurrent_mixed_queries(
            self,
            running_resolver,
            dns_client,
            exact_blocked_domains,
            wildcard_patterns,
            test_domains_should_allow,
            ):
        """Concurrent mix of exact-blocked, wildcard-blocked, and allowed queries."""
        # Build a mixed set of domains from the config
        test_cases = []

        # Exact-blocked
        for domain in exact_blocked_domains[:5]:
            test_cases.append((domain, True, "exact"))

        # Wildcard-blocked
        for pattern in wildcard_patterns[:3]:
            # Create test subdomains for wildcard
            base = pattern.replace("*.", "")
            test_cases.append((f"test.{base}", True, "wildcard"))
            test_cases.append((f"sub.example.{base}", True, "wildcard"))

        # Allowed
        for domain in test_domains_should_allow[:10]:
            test_cases.append((domain, False, "allowed"))

        # Repeat to increase load and randomize
        test_cases = test_cases * 3
        random.shuffle(test_cases)

        print(f"Testing {len(test_cases)} concurrent queries (exact/wildcard/allowed mix)")

        # Fire queries concurrently
        responses = await dns_client.send_concurrent_queries([domain for domain, _, _ in test_cases])

        # Evaluate results
        successful = 0
        blocked_correct = 0
        allowed_correct = 0
        errors = 0

        for i, response in enumerate(responses):
            domain, should_block, category = test_cases[i]

            if isinstance(response, Exception):
                errors += 1
                print(f"✗ Error for {domain} ({category}): {type(response).__name__}")
                continue

            successful += 1

            if should_block:
                if response.is_blocked():
                    blocked_correct += 1
                else:
                    print(f"✗ False negative: {domain} ({category}) not blocked (rcode={response.rcode})")
            else:
                if not response.is_blocked():
                    allowed_correct += 1
                else:
                    print(f"✗ False positive: {domain} ({category}) incorrectly blocked")

        print("✓ Concurrent mixed queries results:")
        print(f"  Total: {len(test_cases)}")
        print(f"  Successful: {successful}")
        print(f"  Blocked correctly: {blocked_correct}")
        print(f"  Allowed correctly: {allowed_correct}")
        print(f"  Errors: {errors}")

        # Validation
        expected_blocked = sum(1 for _, should_block, _ in test_cases if should_block)
        expected_allowed = sum(1 for _, should_block, _ in test_cases if not should_block)

        block_accuracy = (blocked_correct / expected_blocked * 100) if expected_blocked > 0 else 100
        allow_accuracy = (allowed_correct / expected_allowed * 100) if expected_allowed > 0 else 100

        assert block_accuracy > 95.0, f"Block accuracy too low: {block_accuracy:.1f}%"
        assert allow_accuracy > 95.0, f"Allow accuracy too low: {allow_accuracy:.1f}%"

    # --------------------------------------------------------------------- #
    # Sustained load over time
    # --------------------------------------------------------------------- #
    def test_sustained_load(
            self,
            running_resolver,
            dns_client,
            exact_blocked_domains,
            test_domains_should_allow,
            ):
        """Sustained load over time with mixed domain types."""
        duration = 30  # 30 seconds
        start_time = time.time()

        results = {
                "total_queries": 0,
                "successful_queries": 0,
                "blocked_queries": 0,
                "allowed_queries": 0,
                "errors": 0,
                "response_times": [],
                "by_category": {
                        "exact_blocked": 0,
                        "wildcard_blocked": 0,
                        "allowed": 0,
                        },
                }

        # Prepare test domains from the config
        blocked_domains = exact_blocked_domains[:10] if exact_blocked_domains else []
        allowed_domains = (
                test_domains_should_allow[:10] if test_domains_should_allow else ["google.com", "github.com"]
        )
        all_test_domains = blocked_domains + allowed_domains

        if not all_test_domains:
            pytest.skip("No test domains available")

        print(f"Starting sustained load test for {duration}s...")
        print(f"  Blocked domains: {len(blocked_domains)}")
        print(f"  Allowed domains: {len(allowed_domains)}")

        while time.time() - start_time < duration:
            domain = random.choice(all_test_domains)
            # Add a random subdomain for variability
            test_domain = f"test{random.randint(1, 1000)}.{domain}"

            try:
                response = dns_client.send_query(test_domain)
                results["total_queries"] += 1
                results["successful_queries"] += 1
                results["response_times"].append(response.response_time)

                if response.is_blocked():
                    results["blocked_queries"] += 1
                    if domain in blocked_domains:
                        results["by_category"]["exact_blocked"] += 1
                elif response.is_allowed():
                    results["allowed_queries"] += 1
                    if domain in allowed_domains:
                        results["by_category"]["allowed"] += 1

            except Exception as e:
                results["errors"] += 1
                if results["errors"] % 10 == 0:  # Log every 10th error
                    print(f"Error during sustained load: {type(e).__name__}")

            time.sleep(0.05)  # Small pause = higher overall load

        # Compute statistics
        elapsed = time.time() - start_time
        avg_response_time = (
                sum(results["response_times"]) / len(results["response_times"]) if results["response_times"] else 0
        )
        min_response_time = min(results["response_times"]) if results["response_times"] else 0
        max_response_time = max(results["response_times"]) if results["response_times"] else 0
        qps = results["total_queries"] / elapsed if elapsed > 0 else 0

        print("✓ Sustained load test results:")
        print(f"  Duration: {elapsed:.1f}s")
        print(f"  Total queries: {results['total_queries']}")
        print(f"  QPS: {qps:.2f}")
        print(f"  Success rate: {results['successful_queries'] / results['total_queries'] * 100:.1f}%")
        print(f"  Response time: avg={avg_response_time:.3f}s, "
                f"min={min_response_time:.3f}s, max={max_response_time:.3f}s"
                )
        print(f"  Blocked: {results['blocked_queries']}, Allowed: {results['allowed_queries']}")
        print(f"  By category: {results['by_category']}")
        print(f"  Errors: {results['errors']}")

        # Validation
        assert qps > 5, f"Query rate too low: {qps:.2f} QPS"
        success_rate = results["successful_queries"] / results["total_queries"]
        assert success_rate > 0.90, f"Too many failed queries: {(1 - success_rate) * 100:.1f}% failure rate"
        assert running_resolver.process.poll() is None, "Resolver crashed during sustained load"
        assert avg_response_time < 0.5, f"Average response time too high: {avg_response_time:.3f}s"

        # Final health check
        final_stats = running_resolver.get_process_stats()
        print(f"  Final process stats: memory={final_stats.get('memory_mb', 0):.2f}MB, "
                f"cpu={final_stats.get('cpu_percent', 0):.1f}%"
                )

    # --------------------------------------------------------------------- #
    # Filter file: edge cases & categorization
    # --------------------------------------------------------------------- #
    def test_filter_file_edge_cases(self, resolver_manager, temp_filter_file, dns_client):
        """Filter-file edge cases with categorized checks."""
        # Create a complex filter file
        with open(temp_filter_file, "w") as f:
            f.write(
                    """# DNS Filter File - Edge Cases Test
    # Generated for testing

    ### EXACT DOMAINS ###
    # Comment inside section
    blocked-domain.com

    # Empty line
    UPPERCASE-DOMAIN.COM
    mixed-Case.Example.Org

    # Domains with numbers and dashes
    test-123.numeric-domain.com
    domain-with-many-dashes-and-numbers-123-456.com

    # Very long name
    this-is-a-very-long-domain-name-that-tests-parsing-limits.super-long-tld.example.com

    # Edge cases that should still be validated
    single.com
    a.b
    very.deep.subdomain.with.many.labels.example.com

    ### WILDCARD PATTERNS ###
    *.wildcard-test.net
    *.ads.tracker.com
    *.malware.example.org

    # Case variations
    *.UpperCase.Wild.Com
    *.MiXeD-cAsE.test

    # Wildcards with numbers
    *.tracker-123.analytics.net

    # Special cases (should be ignored)
    # invalid@domain.com
    # domain with spaces.com
    # ..invalid.com

    # Trailing comment
    """
                    )

        # Start the resolver with the edge-case filter
        resolver_manager.start_resolver(
                filter_file_costum=temp_filter_file,  # keep original parameter name
                port=15354,
                verbose=True,
                )

        # Fetch filter stats
        filter_stats = resolver_manager.get_filter_stats()
        assert filter_stats and filter_stats.total_entries > 0, "Filter not loaded - empty stats!"

        print("✓ Filter stats:")
        print(f"  Total entries: {filter_stats.total_entries}")
        print(f"  Exact domains: {filter_stats.exact_domains}")
        print(f"  Wildcards: {filter_stats.wildcard_patterns}")
        print(f"  Invalid entries: {filter_stats.invalid_entries}")

        client = DNSTestClient(server_port=15354)
        time.sleep(1)  # Short wait for stabilization

        # Test cases by category
        test_cases = [
                # Exact domains
                ("blocked-domain.com", True, "exact-basic"),
                ("UPPERCASE-DOMAIN.COM", True, "exact-uppercase"),
                ("uppercase-domain.com", True, "exact-case-insensitive"),
                ("Mixed-Case.Example.Org", True, "exact-mixed-case"),
                ("test-123.numeric-domain.com", True, "exact-numeric"),
                (
                        "this-is-a-very-long-domain-name-that-tests-parsing-limits.super-long-tld.example.com",
                        True,
                        "exact-long",
                        ),
                # Wildcard tests
                ("test.wildcard-test.net", True, "wildcard-basic"),
                ("sub.example.wildcard-test.net", True, "wildcard-deep"),
                ("anything.ads.tracker.com", True, "wildcard-ads"),
                ("test.UpperCase.Wild.Com", True, "wildcard-case-insensitive"),
                ("sub.tracker-123.analytics.net", True, "wildcard-numeric"),
                # Should be allowed
                ("allowed-domain.com", False, "allowed-basic"),
                ("google.com", False, "allowed-google"),
                ("github.com", False, "allowed-github"),
                ]

        results = {"exact_blocked": 0, "wildcard_blocked": 0, "allowed": 0, "errors": 0}

        for domain, should_be_blocked, category in test_cases:
            try:
                response = client.send_query(domain)

                if should_be_blocked:
                    if response.is_blocked():
                        if "wildcard" in category:
                            results["wildcard_blocked"] += 1
                        else:
                            results["exact_blocked"] += 1
                        print(f"✓ {category}: {domain} correctly blocked")
                    else:
                        print(f"✗ {category}: {domain} NOT blocked (rcode={response.rcode})")
                        results["errors"] += 1
                else:
                    if response.is_allowed():
                        results["allowed"] += 1
                        print(f"✓ {category}: {domain} correctly allowed")
                    else:
                        print(f"✗ {category}: {domain} incorrectly blocked")
                        results["errors"] += 1

            except Exception as e:
                print(f"✗ Error testing {domain} ({category}): {type(e).__name__}: {e}")
                results["errors"] += 1

        print("✓ Edge cases test summary:")
        print(f"  Exact blocked: {results['exact_blocked']}")
        print(f"  Wildcard blocked: {results['wildcard_blocked']}")
        print(f"  Allowed: {results['allowed']}")
        print(f"  Errors: {results['errors']}")

        # Validation
        assert results["errors"] == 0, f"Edge case handling failed: {results['errors']} errors"

    # --------------------------------------------------------------------- #
    # Burst-load handling
    # --------------------------------------------------------------------- #
    @pytest.mark.asyncio
    async def test_burst_load_handling(
            self, running_resolver, dns_client, exact_blocked_domains, test_domains_should_allow
            ):
        """Handling of burst traffic (waves of concurrent queries)."""
        print("Starting burst load test...")

        # Prepare domain pool
        test_domains = []
        if exact_blocked_domains:
            test_domains.extend(exact_blocked_domains[:10])
        if test_domains_should_allow:
            test_domains.extend(test_domains_should_allow[:10])

        if not test_domains:
            pytest.skip("No test domains available")

        # 5 burst waves
        for wave in range(5):
            print(f"  Wave {wave + 1}/5...")

            # Burst of 50 queries at once
            burst_domains = [random.choice(test_domains) for _ in range(50)]

            start_time = time.time()
            responses = await dns_client.send_concurrent_queries(burst_domains)
            wave_duration = time.time() - start_time

            successful = sum(1 for r in responses if not isinstance(r, Exception))
            errors = sum(1 for r in responses if isinstance(r, Exception))

            print(
                    f"    Wave {wave + 1}: {successful}/{len(burst_domains)} successful in {wave_duration:.2f}s "
                    f"({successful / wave_duration:.1f} QPS)"
                    )

            assert successful > len(burst_domains) * 0.9, f"Wave {wave + 1}: Too many failures"

            # Short pause between waves
            await asyncio.sleep(1)

        # Resolver health check
        assert running_resolver.process.poll() is None, "Resolver crashed during burst load"

        final_stats = running_resolver.get_process_stats()
        print("✓ Burst load test complete - resolver healthy")
        print(f"  Memory: {final_stats.get('memory_mb', 0):.2f} MB")
        print(f"  CPU: {final_stats.get('cpu_percent', 0):.1f}%")

    # --------------------------------------------------------------------- #
    # Categorized batch testing
    # --------------------------------------------------------------------- #
    def test_categorized_batch_testing(
            self, running_resolver, dns_client, test_domains_should_block, test_domains_should_allow
            ):
        """Batch testing by category (exact, wildcard, subdomain, allowed)."""
        print("Starting categorized batch testing...")

        # Exact domains
        exact_domains = test_domains_should_block.get("exact", [])
        if exact_domains:
            result = dns_client.test_exact_domain_blocking(exact_domains)
            print(f"✓ Exact domains: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Exact domain block rate too low: {result.block_rate:.1f}%"

        # Wildcards
        wildcard_domains = test_domains_should_block.get("wildcards", [])
        if wildcard_domains:
            result = dns_client.test_wildcard_blocking(wildcard_domains)
            print(f"✓ Wildcards: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Wildcard block rate too low: {result.block_rate:.1f}%"

        # Subdomains
        subdomain_domains = test_domains_should_block.get("subdomains", [])
        if subdomain_domains:
            result = dns_client.test_exact_domain_blocking(subdomain_domains)
            print(f"✓ Subdomains: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Subdomain block rate too low: {result.block_rate:.1f}%"

        # Allowed domains
        if test_domains_should_allow:
            result = dns_client.test_allowed_domains(test_domains_should_allow)
            print(f"✓ Allowed: {result.allowed}/{result.total} allowed ({result.success_rate:.1f}%)")
            assert result.blocked == 0, f"False positives detected: {result.blocked} domains"

    # --------------------------------------------------------------------- #
    # Mixed query types under load (blocked domain)
    # --------------------------------------------------------------------- #
    def test_mixed_query_types_under_load(self, running_resolver, dns_client, exact_blocked_domains):
        """Different query types under load against a blocked domain."""
        if not exact_blocked_domains:
            pytest.skip("No blocked domains available")

        query_types = [(1, "A"), (28, "AAAA"), (15, "MX"), (2, "NS"), (5, "CNAME"), (16, "TXT")]

        blocked_domain = exact_blocked_domains[0]
        results = {"total": 0, "blocked": 0, "notimp": 0, "errors": 0}

        print(f"Testing multiple query types for {blocked_domain}...")

        for _ in range(10):  # 10 iterations
            for qtype, name in query_types:
                try:
                    response = dns_client.send_query(blocked_domain, qtype=qtype)
                    results["total"] += 1
                    if qtype == 1:  # A
                        assert response.is_blocked(), f"{name} query should be blocked"
                        assert response.rcode == DNSRCode.REFUSED, f"{name} should return REFUSED"
                        results["blocked"] += 1
                    else:
                        assert response.rcode == DNSRCode.NOTIMP, f"{name} should return NOTIMP"
                        results["notimp"] += 1
                except Exception as e:
                    results["errors"] += 1
                    print(f"✗ Error for query type {name}: {type(e).__name__}")

        print("✓ Mixed query types results:")
        print(f"  Total: {results['total']}")
        print(f"  Blocked (A): {results['blocked']}")
        print(f"  NOTIMP (other): {results['notimp']}")
        print(f"  Errors: {results['errors']}")

        assert results["blocked"] == 10, f"Block rate for A too low: {results['blocked']}/10"
        assert results["notimp"] == 10 * (len(query_types) - 1), f"NOTIMP count incorrect: {results['notimp']}"
        assert running_resolver.process.poll() is None

### end of file ComplexFunctionalityTest.py ###
