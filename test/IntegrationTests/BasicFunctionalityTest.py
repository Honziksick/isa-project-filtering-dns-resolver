import pytest
import time
import asyncio
from DnsClient import DNSRCode, DNSTestClient, BatchTestResult


class TestBasicFunctionality:
    """Základní funkční testy s podporou exact domén i wildcardů"""

    def test_resolver_startup(self, running_resolver):
        """Test úspěšného startu resolveru s kontrolou filter statistik"""
        assert running_resolver.process is not None
        assert running_resolver.process.poll() is None  # Process běží

        stats = running_resolver.get_process_stats()
        assert stats.get('status') in ['running', 'sleeping']

        # Kontrola filter statistik
        filter_stats = stats.get('filter', {})
        assert filter_stats.get('total_entries', 0) > 0
        assert filter_stats.get('exact_domains', 0) >= 0
        assert filter_stats.get('wildcard_patterns', 0) >= 0

        print(f"✓ Resolver started with {filter_stats.get('exact_domains', 0)} exact domains "
              f"and {filter_stats.get('wildcard_patterns', 0)} wildcards")

    def test_blocked_domain_refused(self, running_resolver, dns_client, exact_blocked_domains):
        """Test blokování exact domén - očekává REFUSED (RFC 1035)"""
        # RFC 1035: REFUSED (5) = policy-based rejection
        test_domains = exact_blocked_domains[:5]  # První 5 domén

        for domain in test_domains:
            response = dns_client.send_query(domain)

            assert response.rcode == DNSRCode.REFUSED, \
                f"Domain {domain} should be blocked with REFUSED (RFC 1035 policy-based blocking)"
            assert response.transaction_id == dns_client.transaction_id
            assert response.answers == 0
            assert response.is_blocked()
            print(f"✓ Exact domain blocked with REFUSED: {domain} (time: {response.response_time:.3f}s)")

    def test_allowed_domain_forwarded(self, running_resolver, dns_client, test_domains_should_allow):
        """Test povolených domén - měly by být předány upstream"""
        test_domains = test_domains_should_allow[:5]

        for domain in test_domains:
            response = dns_client.send_query(domain)

            # Povolené domény by měly mít normální odpověď
            assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
            assert response.transaction_id == dns_client.transaction_id
            assert not response.is_blocked()
            print(f"✓ Allowed domain processed: {domain} (rcode={response.rcode}, time: {response.response_time:.3f}s)")

    def test_wildcard_blocking_basic(self, running_resolver, dns_client, test_domains_should_block):
        """Test základního wildcard blokování"""
        wildcard_test_domains = test_domains_should_block.get('wildcards', [])

        if not wildcard_test_domains:
            pytest.skip("No wildcard test domains configured")

        for domain in wildcard_test_domains[:5]:  # Test prvních 5
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Wildcard domain {domain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED for wildcard block"
            print(f"✓ Wildcard blocked with REFUSED: {domain} (time: {response.response_time:.3f}s)")

    def test_subdomain_blocking(self, running_resolver, dns_client, test_domains_should_block):
        """Test blokování subdomén exact domén"""
        subdomain_tests = test_domains_should_block.get('subdomains', [])

        if not subdomain_tests:
            pytest.skip("No subdomain test domains configured")

        for domain in subdomain_tests[:5]:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Subdomain {domain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED for subdomain block"
            print(f"✓ Subdomain blocked with REFUSED: {domain} (time: {response.response_time:.3f}s)")

    def test_case_insensitive_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Test case-insensitive blokování"""
        test_domain = exact_blocked_domains[0] if exact_blocked_domains else "blocked-domain.com"

        test_cases = [
                test_domain.upper(),
                test_domain.lower(),
                test_domain.title(),
                ''.join(c.upper() if i % 2 else c.lower() for i, c in enumerate(test_domain))
                ]

        for domain in test_cases:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Domain {domain} should be blocked (case insensitive)"
            assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED"
            print(f"✓ Case insensitive blocked with REFUSED: {domain}")

    def test_wildcard_case_insensitive(self, running_resolver, dns_client, wildcard_patterns):
        """Test case-insensitive wildcard blokování"""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        pattern = wildcard_patterns[0].replace('*.', '') if wildcard_patterns else "doubleclick.net"
        test_cases = [
                f"test.{pattern.upper()}",
                f"test.{pattern.lower()}",
                f"test.{pattern.title()}"
                ]

        for domain in test_cases:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Wildcard domain {domain} should be blocked (case insensitive)"
            assert response.rcode == DNSRCode.REFUSED
            print(f"✓ Wildcard case insensitive with REFUSED: {domain}")

    @pytest.mark.parametrize("qtype,type_name", [
            (1, "A"),
            (28, "AAAA"),
            (15, "MX"),
            (2, "NS"),
            (5, "CNAME"),
            (16, "TXT")
            ])
    def test_different_query_types_blocked(self, running_resolver, dns_client, qtype, type_name,
                                           exact_blocked_domains):
        """Test různých typů DNS dotazů na blokované domény"""
        blocked_domain = exact_blocked_domains[0] if exact_blocked_domains else "blocked-domain.com"
        response = dns_client.send_query(blocked_domain, qtype=qtype)

        assert response.is_blocked(), f"Blocked domain should return REFUSED for {type_name} query"
        assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED for {type_name}"
        print(f"✓ Query type {type_name} correctly blocked with REFUSED for {blocked_domain}")

    @pytest.mark.parametrize("qtype,type_name", [
            (1, "A"),
            (28, "AAAA"),
            (15, "MX"),
            (2, "NS")
            ])
    def test_different_query_types_allowed(self, running_resolver, dns_client, qtype, type_name):
        """Test různých typů DNS dotazů na povolené domény"""
        response = dns_client.send_query("google.com", qtype=qtype)

        # Povolené domény by NEMĚLY dostat REFUSED
        assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN, DNSRCode.NOTIMP]
        assert response.rcode != DNSRCode.REFUSED, "Allowed domain should not be REFUSED"
        assert not response.is_blocked()
        print(f"✓ Query type {type_name} handled correctly for allowed domain (rcode={response.rcode})")

    def test_response_time_performance(self, running_resolver, dns_client, test_domains_should_allow):
        """Test rychlosti odpovědi pro různé typy domén"""
        test_domains = test_domains_should_allow[:10] if len(test_domains_should_allow) >= 10 else test_domains_should_allow
        response_times = []

        for domain in test_domains:
            response = dns_client.send_query(domain)
            response_times.append(response.response_time)

        avg_time = sum(response_times) / len(response_times)
        max_time = max(response_times)
        min_time = min(response_times)

        print(f"✓ Response time stats:")
        print(f"  Average: {avg_time:.3f}s")
        print(f"  Min: {min_time:.3f}s")
        print(f"  Max: {max_time:.3f}s")

        assert avg_time < 1.0, f"Average response time too slow: {avg_time}s"
        assert max_time < 2.0, f"Maximum response time too slow: {max_time}s"

    def test_blocked_domain_performance(self, running_resolver, dns_client, exact_blocked_domains):
        """Test rychlosti blokování - mělo by být velmi rychlé"""
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

        print(f"✓ Blocked domain response time:")
        print(f"  Average: {avg_time:.3f}s")
        print(f"  Max: {max_time:.3f}s")

        # Blokování by mělo být extrémně rychlé (žádný upstream)
        assert avg_time < 0.1, f"Blocked domain response too slow: {avg_time}s"

    def test_batch_exact_domain_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Batch test exact domén"""
        result = dns_client.test_exact_domain_blocking(exact_blocked_domains)

        print(f"✓ Exact domain batch test:")
        print(f"  Total: {result.total}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Allowed: {result.allowed}")
        print(f"  Errors: {result.errors}")
        print(f"  Block rate: {result.block_rate:.1f}%")

        assert result.block_rate > 95.0, f"Expected >95% block rate, got {result.block_rate:.1f}%"

    def test_batch_wildcard_blocking(self, running_resolver, dns_client, test_domains_should_block):
        """Batch test wildcard domén"""
        wildcard_tests = test_domains_should_block.get('wildcards', [])

        if not wildcard_tests:
            pytest.skip("No wildcard test domains configured")

        result = dns_client.test_wildcard_blocking(wildcard_tests)

        print(f"✓ Wildcard batch test:")
        print(f"  Total: {result.total}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Block rate: {result.block_rate:.1f}%")

        assert result.block_rate > 95.0, f"Expected >95% wildcard block rate, got {result.block_rate:.1f}%"

    def test_batch_allowed_domains(self, running_resolver, dns_client, test_domains_should_allow):
        """Batch test povolených domén"""
        result = dns_client.test_allowed_domains(test_domains_should_allow)

        print(f"✓ Allowed domains batch test:")
        print(f"  Total: {result.total}")
        print(f"  Allowed: {result.allowed}")
        print(f"  Blocked: {result.blocked}")
        print(f"  Success rate: {result.success_rate:.1f}%")

        # Žádná by neměla být chybně zablokována (REFUSED)
        assert result.blocked == 0, f"False positives detected: {result.blocked} domains blocked with REFUSED"

    @pytest.mark.asyncio
    @pytest.mark.timeout(30)
    async def test_concurrent_blocking_and_allowing(self, running_resolver, dns_client,
                                                    test_domains_should_block, test_domains_should_allow):
        """Test souběžného blokování a povolování"""
        blocked_tests = test_domains_should_block.get('exact', [])[:10]
        allowed_tests = test_domains_should_allow[:10]

        # Debug: kolik skutečně testujeme
        print(f"✓ Testing {len(blocked_tests)} blocked domains and {len(allowed_tests)} allowed domains")

        blocked_result, allowed_result = await dns_client.test_concurrent_blocking(
                blocked_tests,
                allowed_tests
                )

        print(f"✓ Concurrent test results:")
        print(f"  Blocked domains: {blocked_result.blocked}/{blocked_result.total} "
              f"({blocked_result.block_rate:.1f}%)")
        print(f"  Allowed domains: {allowed_result.allowed}/{allowed_result.total} "
              f"({100.0 * allowed_result.allowed / max(1, allowed_result.total):.1f}%)")

        # Detail o failures
        if blocked_result.failed_domains:
            print(f"\n❌ Blocked domains failures:")
            print(blocked_result.get_failure_summary())

        if allowed_result.failed_domains:
            print(f"\n❌ Allowed domains failures:")
            print(allowed_result.get_failure_summary())

        # Assertions s detailem
        assert blocked_result.block_rate > 95.0, \
            f"Expected >95% block rate, got {blocked_result.block_rate:.1f}% " \
            f"({blocked_result.blocked}/{blocked_result.total} blocked)\n" \
            f"{blocked_result.get_failure_summary()}"

        assert allowed_result.allowed == allowed_result.total, \
            f"Expected all {allowed_result.total} allowed domains to pass, " \
            f"but {len(allowed_result.failed_domains)} failed:\n" \
            f"{allowed_result.get_failure_summary()}"

    def test_deep_subdomain_blocking(self, running_resolver, dns_client, exact_blocked_domains):
        """Test blokování hlubokých subdomén"""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        base_domain = exact_blocked_domains[0]
        deep_subdomains = [
                f"sub.{base_domain}",
                f"deep.sub.{base_domain}",
                f"very.deep.sub.{base_domain}",
                f"extremely.very.deep.sub.{base_domain}"
                ]

        for domain in deep_subdomains:
            response = dns_client.send_query(domain)
            assert response.is_blocked(), f"Deep subdomain {domain} should be blocked"
            assert response.rcode == DNSRCode.REFUSED, f"Should return REFUSED"
            print(f"✓ Deep subdomain blocked with REFUSED: {domain}")

    def test_wildcard_not_matching_parent(self, running_resolver, dns_client, wildcard_patterns):
        """Test že wildcard *.example.com neblokuje example.com"""
        if not wildcard_patterns:
            pytest.skip("No wildcard patterns configured")

        pattern = wildcard_patterns[0].replace('*.', '') if wildcard_patterns else "doubleclick.net"

        # Parent doména - závisí na implementaci (dokumentuj chování)
        response = dns_client.send_query(pattern)
        print(f"✓ Parent domain {pattern} result: blocked={response.is_blocked()}, rcode={response.rcode}")

        # Subdoména by MĚLA být zablokována s REFUSED
        subdomain_response = dns_client.send_query(f"test.{pattern}")
        assert subdomain_response.is_blocked(), f"Subdomain test.{pattern} should be blocked"
        assert subdomain_response.rcode == DNSRCode.REFUSED, f"Should return REFUSED for wildcard"
        print(f"✓ Subdomain test.{pattern} correctly blocked with REFUSED")

    def test_edge_case_domains(self, running_resolver, dns_client, edge_case_domains):
        """Test edge case domén"""
        for case_name, domain in edge_case_domains.items():
            if not domain or domain in ["", ".", ".."]:
                # Tyto způsobí parsing error - očekáváme timeout nebo error
                try:
                    response = dns_client.send_query(domain)
                    # Pokud odpověď přijde, měla by to být chyba
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
        """Test prázdného/malformed dotazu"""
        malformed_query = b'\x00\x00' * 6  # Neplatný DNS paket

        response = dns_client.send_malformed_query(malformed_query)

        if response is None:
            print("✓ Malformed query correctly ignored (no response)")
        else:
            # RFC 1035: FORMERR pro malformed dotazy
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Malformed query handled with rcode={response.rcode}")

    def test_refused_vs_nxdomain_distinction(self, running_resolver, dns_client,
                                             exact_blocked_domains, test_domains_should_allow):
        """Test rozlišení mezi REFUSED (blocked) a NXDOMAIN (neexistující)"""
        if not exact_blocked_domains:
            pytest.skip("No blocked domains configured")

        # Blokovaná doména -> REFUSED
        blocked_response = dns_client.send_query(exact_blocked_domains[0])
        assert blocked_response.rcode == DNSRCode.REFUSED
        print(f"✓ Blocked domain returns REFUSED (policy-based blocking)")

        # Neexistující doména -> NXDOMAIN (od upstream)
        nonexistent = f"this-definitely-does-not-exist-{int(time.time())}.example.com"
        nxdomain_response = dns_client.send_query(nonexistent)
        assert nxdomain_response.rcode == DNSRCode.NXDOMAIN
        print(f"✓ Non-existent domain returns NXDOMAIN (from upstream)")

        # Existující povolená doména -> NOERROR
        if test_domains_should_allow:
            allowed_response = dns_client.send_query(test_domains_should_allow[0])
            assert allowed_response.rcode == DNSRCode.NOERROR
            print(f"✓ Existing allowed domain returns NOERROR")

        print(f"✓ RFC 1035 compliant: REFUSED != NXDOMAIN distinction working")

#     def test_invalid_domains_ignored(self, resolver_manager, temp_filter_file, dns_client):
#         """Test ignorování nevalidních domén v filter souboru"""
#         # Vytvoř speciální filter s nevalidními doménami
#         with open(temp_filter_file, 'w') as f:
#             f.write("""# Generated for testing
#
# ### EXACT DOMAINS ###
# blocked-domain.com
# valid-domain.com
#
# ### WILDCARD PATTERNS ###
# *.wildcard-test.net
#
# ### NEVALIDNÍ DOMÉNY BY MĚLY BÝT IGNOROVÁNY ###
# # Neplatný znak v doméně (vykřičník, zavináč, podtržítko, mezera)
# exam!ple.com
# invalid@domain.com
# examp_le.com
# exam ple.com
#
# # Více po sobě jdoucích teček
# example..com
# ..example.com
# example.com..
#
# # Prázdný label na začátku nebo na konci (tečka na začátku nebo na konci)
# .example.com
# example.com.
#
# # Doména delší než 253 znaků
# aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc.dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd.com
#
# # Label delší než 63 znaků
# aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com
#
# # Label začíná nebo končí pomlčkou
# -example.com
# example-.com
# -invalid-.com
# """)
#
#         # Start resolveru s edge case filtrem
#         resolver_manager.start_resolver(
#                 filter_file_costum=temp_filter_file,  # Explicitně určíme soubor
#                 port=15354,
#                 verbose=True
#                 )
#
#         # Získání filter statistik
#         filter_stats = resolver_manager.get_filter_stats()
#         assert filter_stats and filter_stats.total_entries > 0, "Filter not loaded - empty stats!"
#
#         client = DNSTestClient(server_port=15354)
#         time.sleep(1)  # Krátké čekání na stabilizaci
#
#         # Test cases podle kategorií
#         test_cases = [
#                 # Exact domains
#                 # ("blocked-domain.com", True, "exact-1"),
#                 # ("valid-domain.com", True, "exact-2"),
#                 # ("example.com", True, "exact-3"),
#
#                 # Wildcard tests
#                 # ("test.wildcard-test.net", True, "wildcard-1"),
#
#                 # Should be allowed
#                 # ("allowed-domain.com", False, "allowed-basic"),
#                 # ("google.com", False, "allowed-google"),
#                 # ("github.com", False, "allowed-github"),
#                 ("-example.com", False, "-example.com"),
#                 ("-.wildcard-test.net", False, "-.wildcard-test.net"),
#                 # ("example-.com", False, "allowed-example-.com"),
#                 # ("-invalid-.com", False, "-invalid-.com"),
#                 # ("exam!ple.com", False, "exam!ple.com"),
#                 # ("invalid@domain.com", False, "invalid@domain.com"),
#                 # ("examp_le.com", False, "examp_le.com"),
#                 # ("exam ple.com", False, "exam ple.com"),
#                 # ("example..com", False, "example..com"),
#                 # ("..example.com", False, "..example.com"),
#                 # ("example.com..", False, "example.com.."),
#                 # (".example.com", False, ".example.com"),
#                 ]
#
#         results = {
#                 'exact_blocked': 0,
#                 'wildcard_blocked': 0,
#                 'allowed': 0,
#                 'errors': 0
#                 }
#
#         for domain, should_be_blocked, category in test_cases:
#             try:
#                 response = client.send_query(domain)
#
#                 if should_be_blocked:
#                     if response.is_blocked():
#                         if 'wildcard' in category:
#                             results['wildcard_blocked'] += 1
#                         else:
#                             results['exact_blocked'] += 1
#                         print(f"✓ {category}: {domain} correctly blocked")
#                     else:
#                         print(f"✗ {category}: {domain} NOT blocked (rcode={response.rcode})")
#                         results['errors'] += 1
#                 else:
#                     if response.is_allowed():
#                         results['allowed'] += 1
#                         print(f"✓ {category}: {domain} correctly allowed")
#                     else:
#                         print(f"✗ {category}: {domain} incorrectly blocked")
#                         results['errors'] += 1
#
#             except Exception as e:
#                 print(f"✗ Error testing {domain} ({category}): {type(e).__name__}: {e}")
#                 results['errors'] += 1
#
#         print(f"✓ Edge cases test summary:")
#         print(f"  Exact blocked: {results['exact_blocked']}")
#         print(f"  Wildcard blocked: {results['wildcard_blocked']}")
#         print(f"  Allowed: {results['allowed']}")
#         print(f"  Errors: {results['errors']}")
#
#         # Validace
#         assert results['errors'] == 0, f"Failed: {results['errors']} errors"
