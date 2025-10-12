import pytest
import asyncio
import time
import random
import threading
from concurrent.futures import ThreadPoolExecutor
from DnsClient import DNSRCode, DNSTestClient, BatchTestResult


class TestComplexFunctionality:
    """Komplexní testovací scénáře s podporou nového configu"""

    @pytest.mark.asyncio
    async def test_concurrent_mixed_queries(self, running_resolver, dns_client,
                                            exact_blocked_domains, wildcard_patterns,
                                            test_domains_should_allow):
        """Test smíchané současné dotazy (exact, wildcard, povolené)"""
        # Sestavení mix domén z configu
        test_cases = []

        # Exact blokované
        for domain in exact_blocked_domains[:5]:
            test_cases.append((domain, True, "exact"))

        # Wildcard blokované
        for pattern in wildcard_patterns[:3]:
            # Vytvoř testovací subdomény pro wildcard
            base = pattern.replace('*.', '')
            test_cases.append((f"test.{base}", True, "wildcard"))
            test_cases.append((f"sub.example.{base}", True, "wildcard"))

        # Povolené
        for domain in test_domains_should_allow[:10]:
            test_cases.append((domain, False, "allowed"))

        # Opakování pro zvýšení zátěže
        test_cases = test_cases * 3
        random.shuffle(test_cases)

        print(f"Testing {len(test_cases)} concurrent queries (exact/wildcard/allowed mix)")

        # Asynchronní dotazy
        responses = await dns_client.send_concurrent_queries([domain for domain, _, _ in test_cases])

        # Vyhodnocení výsledků
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

        print(f"✓ Concurrent mixed queries results:")
        print(f"  Total: {len(test_cases)}")
        print(f"  Successful: {successful}")
        print(f"  Blocked correctly: {blocked_correct}")
        print(f"  Allowed correctly: {allowed_correct}")
        print(f"  Errors: {errors}")

        # Validace
        expected_blocked = sum(1 for _, should_block, _ in test_cases if should_block)
        expected_allowed = sum(1 for _, should_block, _ in test_cases if not should_block)

        block_accuracy = (blocked_correct / expected_blocked * 100) if expected_blocked > 0 else 100
        allow_accuracy = (allowed_correct / expected_allowed * 100) if expected_allowed > 0 else 100

        assert block_accuracy > 95.0, f"Block accuracy too low: {block_accuracy:.1f}%"
        assert allow_accuracy > 95.0, f"Allow accuracy too low: {allow_accuracy:.1f}%"

    def test_sustained_load(self, running_resolver, dns_client,
                            exact_blocked_domains, test_domains_should_allow):
        """Test dlouhodobé zátěže s různými typy domén"""
        duration = 30  # 30 sekund
        start_time = time.time()

        results = {
                'total_queries': 0,
                'successful_queries': 0,
                'blocked_queries': 0,
                'allowed_queries': 0,
                'errors': 0,
                'response_times': [],
                'by_category': {
                        'exact_blocked': 0,
                        'wildcard_blocked': 0,
                        'allowed': 0
                        }
                }

        # Připravení test domén z configu
        blocked_domains = exact_blocked_domains[:10] if exact_blocked_domains else []
        allowed_domains = test_domains_should_allow[:10] if test_domains_should_allow else ["google.com", "github.com"]
        all_test_domains = blocked_domains + allowed_domains

        if not all_test_domains:
            pytest.skip("No test domains available")

        print(f"Starting sustained load test for {duration}s...")
        print(f"  Blocked domains: {len(blocked_domains)}")
        print(f"  Allowed domains: {len(allowed_domains)}")

        while time.time() - start_time < duration:
            domain = random.choice(all_test_domains)
            # Přidání random subdomény pro variabilitu
            test_domain = f"test{random.randint(1, 1000)}.{domain}"

            try:
                response = dns_client.send_query(test_domain)
                results['total_queries'] += 1
                results['successful_queries'] += 1
                results['response_times'].append(response.response_time)

                if response.is_blocked():
                    results['blocked_queries'] += 1
                    if domain in blocked_domains:
                        results['by_category']['exact_blocked'] += 1
                elif response.is_allowed():
                    results['allowed_queries'] += 1
                    if domain in allowed_domains:
                        results['by_category']['allowed'] += 1

            except Exception as e:
                results['errors'] += 1
                if results['errors'] % 10 == 0:  # Log každou 10. chybu
                    print(f"Error during sustained load: {type(e).__name__}")

            time.sleep(0.05)  # Menší pauza = vyšší zátěž

        # Vypočet statistik
        elapsed = time.time() - start_time
        avg_response_time = sum(results['response_times']) / len(results['response_times']) if results['response_times'] else 0
        min_response_time = min(results['response_times']) if results['response_times'] else 0
        max_response_time = max(results['response_times']) if results['response_times'] else 0
        qps = results['total_queries'] / elapsed

        print(f"✓ Sustained load test results:")
        print(f"  Duration: {elapsed:.1f}s")
        print(f"  Total queries: {results['total_queries']}")
        print(f"  QPS: {qps:.2f}")
        print(f"  Success rate: {results['successful_queries']/results['total_queries']*100:.1f}%")
        print(f"  Response time: avg={avg_response_time:.3f}s, min={min_response_time:.3f}s, max={max_response_time:.3f}s")
        print(f"  Blocked: {results['blocked_queries']}, Allowed: {results['allowed_queries']}")
        print(f"  By category: {results['by_category']}")
        print(f"  Errors: {results['errors']}")

        # Validace
        assert qps > 5, f"Query rate too low: {qps:.2f} QPS"
        success_rate = results['successful_queries'] / results['total_queries']
        assert success_rate > 0.90, f"Too many failed queries: {(1-success_rate)*100:.1f}% failure rate"
        assert running_resolver.process.poll() is None, "Resolver crashed during sustained load"
        assert avg_response_time < 0.5, f"Average response time too high: {avg_response_time:.3f}s"

        # Kontrola process health
        final_stats = running_resolver.get_process_stats()
        print(f"  Final process stats: memory={final_stats.get('memory_mb', 0):.2f}MB, "
              f"cpu={final_stats.get('cpu_percent', 0):.1f}%")

    def test_filter_file_edge_cases(self, resolver_manager, temp_filter_file, dns_client):
        """Test edge cases v filter souboru s kategorizací"""
        # Vytvoření komplexního filter souboru
        with open(temp_filter_file, 'w') as f:
            f.write("""# DNS Filter File - Edge Cases Test
# Generated for testing

### EXACT DOMAINS ###
# Komentář v sekci
blocked-domain.com

# Prázdný řádek
UPPERCASE-DOMAIN.COM
mixed-Case.Example.Org

# Domény s čísly a pomlčkami
test-123.numeric-domain.com
domain-with-many-dashes-and-numbers-123-456.com

# Velmi dlouhé jméno
this-is-a-very-long-domain-name-that-tests-parsing-limits.super-long-tld.example.com

# Edge cases které by měly být validovány
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

# Wildcard s čísly
*.tracker-123.analytics.net

# Speciální případy (měly by být ignorovány)
# invalid@domain.com
# domain with spaces.com
# ..invalid.com

# Komentář na konci
""")

        # Start resolveru s edge case filtrem
        resolver_manager.start_resolver(
                filter_file_costum=temp_filter_file,  # Explicitně určíme soubor
                port=15354,
                verbose=True
                )

        # Získání filter statistik
        filter_stats = resolver_manager.get_filter_stats()
        assert filter_stats and filter_stats.total_entries > 0, "Filter not loaded - empty stats!"

        print(f"✓ Filter stats:")
        print(f"  Total entries: {filter_stats.total_entries}")
        print(f"  Exact domains: {filter_stats.exact_domains}")
        print(f"  Wildcards: {filter_stats.wildcard_patterns}")
        print(f"  Invalid entries: {filter_stats.invalid_entries}")

        client = DNSTestClient(server_port=15354)
        time.sleep(1)  # Krátké čekání na stabilizaci

        # Test cases podle kategorií
        test_cases = [
                # Exact domains
                ("blocked-domain.com", True, "exact-basic"),
                ("UPPERCASE-DOMAIN.COM", True, "exact-uppercase"),
                ("uppercase-domain.com", True, "exact-case-insensitive"),
                ("Mixed-Case.Example.Org", True, "exact-mixed-case"),
                ("test-123.numeric-domain.com", True, "exact-numeric"),
                ("this-is-a-very-long-domain-name-that-tests-parsing-limits.super-long-tld.example.com", True, "exact-long"),

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

        results = {
                'exact_blocked': 0,
                'wildcard_blocked': 0,
                'allowed': 0,
                'errors': 0
                }

        for domain, should_be_blocked, category in test_cases:
            try:
                response = client.send_query(domain)

                if should_be_blocked:
                    if response.is_blocked():
                        if 'wildcard' in category:
                            results['wildcard_blocked'] += 1
                        else:
                            results['exact_blocked'] += 1
                        print(f"✓ {category}: {domain} correctly blocked")
                    else:
                        print(f"✗ {category}: {domain} NOT blocked (rcode={response.rcode})")
                        results['errors'] += 1
                else:
                    if response.is_allowed():
                        results['allowed'] += 1
                        print(f"✓ {category}: {domain} correctly allowed")
                    else:
                        print(f"✗ {category}: {domain} incorrectly blocked")
                        results['errors'] += 1

            except Exception as e:
                print(f"✗ Error testing {domain} ({category}): {type(e).__name__}: {e}")
                results['errors'] += 1

        print(f"✓ Edge cases test summary:")
        print(f"  Exact blocked: {results['exact_blocked']}")
        print(f"  Wildcard blocked: {results['wildcard_blocked']}")
        print(f"  Allowed: {results['allowed']}")
        print(f"  Errors: {results['errors']}")

        # Validace
        assert results['errors'] == 0, f"Edge case handling failed: {results['errors']} errors"

    @pytest.mark.asyncio
    async def test_burst_load_handling(self, running_resolver, dns_client,
                                       exact_blocked_domains, test_domains_should_allow):
        """Test zvládnutí burst zátěže"""
        print("Starting burst load test...")

        # Připravení domén
        test_domains = []
        if exact_blocked_domains:
            test_domains.extend(exact_blocked_domains[:10])
        if test_domains_should_allow:
            test_domains.extend(test_domains_should_allow[:10])

        if not test_domains:
            pytest.skip("No test domains available")

        # 5 burst waves
        for wave in range(5):
            print(f"  Wave {wave+1}/5...")

            # Burst 50 dotazů najednou
            burst_domains = [random.choice(test_domains) for _ in range(50)]

            start_time = time.time()
            responses = await dns_client.send_concurrent_queries(burst_domains)
            wave_duration = time.time() - start_time

            successful = sum(1 for r in responses if not isinstance(r, Exception))
            errors = sum(1 for r in responses if isinstance(r, Exception))

            print(f"    Wave {wave+1}: {successful}/{len(burst_domains)} successful in {wave_duration:.2f}s "
                  f"({successful/wave_duration:.1f} QPS)")

            assert successful > len(burst_domains) * 0.9, f"Wave {wave+1}: Too many failures"

            # Krátká pauza mezi waves
            await asyncio.sleep(1)

        # Kontrola zdraví resolveru
        assert running_resolver.process.poll() is None, "Resolver crashed during burst load"

        final_stats = running_resolver.get_process_stats()
        print(f"✓ Burst load test complete - resolver healthy")
        print(f"  Memory: {final_stats.get('memory_mb', 0):.2f} MB")
        print(f"  CPU: {final_stats.get('cpu_percent', 0):.1f}%")

    def test_categorized_batch_testing(self, running_resolver, dns_client,
                                       test_domains_should_block, test_domains_should_allow):
        """Test batch testování podle kategorií"""
        print("Starting categorized batch testing...")

        # Test exact domén
        exact_domains = test_domains_should_block.get('exact', [])
        if exact_domains:
            result = dns_client.test_exact_domain_blocking(exact_domains)
            print(f"✓ Exact domains: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Exact domain block rate too low: {result.block_rate:.1f}%"

        # Test wildcards
        wildcard_domains = test_domains_should_block.get('wildcards', [])
        if wildcard_domains:
            result = dns_client.test_wildcard_blocking(wildcard_domains)
            print(f"✓ Wildcards: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Wildcard block rate too low: {result.block_rate:.1f}%"

        # Test subdomén
        subdomain_domains = test_domains_should_block.get('subdomains', [])
        if subdomain_domains:
            result = dns_client.test_exact_domain_blocking(subdomain_domains)
            print(f"✓ Subdomains: {result.blocked}/{result.total} blocked ({result.block_rate:.1f}%)")
            assert result.block_rate > 95.0, f"Subdomain block rate too low: {result.block_rate:.1f}%"

        # Test povolených domén
        if test_domains_should_allow:
            result = dns_client.test_allowed_domains(test_domains_should_allow)
            print(f"✓ Allowed: {result.allowed}/{result.total} allowed ({result.success_rate:.1f}%)")
            assert result.blocked == 0, f"False positives detected: {result.blocked} domains"

    def test_mixed_query_types_under_load(self, running_resolver, dns_client,
                                          exact_blocked_domains):
        """Test různých query typů pod zátěží"""
        if not exact_blocked_domains:
            pytest.skip("No blocked domains available")

        query_types = [
                (1, "A"),
                (28, "AAAA"),
                (15, "MX"),
                (2, "NS"),
                (5, "CNAME"),
                (16, "TXT")
                ]

        blocked_domain = exact_blocked_domains[0]
        results = {'total': 0, 'blocked': 0, 'errors': 0}

        print(f"Testing multiple query types for {blocked_domain}...")

        # Opakované dotazy různých typů
        for _ in range(10):  # 10 iterací
            for qtype, name in query_types:
                try:
                    response = dns_client.send_query(blocked_domain, qtype=qtype)
                    results['total'] += 1
                    if response.is_blocked():
                        results['blocked'] += 1
                except Exception as e:
                    results['errors'] += 1
                    print(f"✗ Error for query type {name}: {type(e).__name__}")

        block_rate = (results['blocked'] / results['total'] * 100) if results['total'] > 0 else 0

        print(f"✓ Mixed query types results:")
        print(f"  Total: {results['total']}")
        print(f"  Blocked: {results['blocked']}")
        print(f"  Block rate: {block_rate:.1f}%")
        print(f"  Errors: {results['errors']}")

        assert block_rate > 95.0, f"Block rate too low: {block_rate:.1f}%"
        assert running_resolver.process.poll() is None
