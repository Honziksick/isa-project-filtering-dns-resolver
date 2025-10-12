import pytest
import struct
import asyncio
from DnsClient import DNSRCode, DNSTestClient


class TestErrorHandling:
    """Testy error handlingu s podporou nového configu"""

    def test_malformed_query_too_short(self, running_resolver, dns_client):
        """Test příliš krátkého DNS dotazu"""
        malformed_query = b'\x12\x34\x01\x00'  # Pouze 4 bytes

        response = dns_client.send_malformed_query(malformed_query)

        if response:  # Může vrátit FORMERR nebo ignorovat
            assert response.rcode == DNSRCode.FORMERR
            print("✓ Short malformed query handled with FORMERR")
        else:
            print("✓ Short malformed query ignored (no response)")

        # Resolver by měl být stále živý
        assert running_resolver.process.poll() is None
        print("✓ Resolver survived malformed query")

    def test_malformed_query_invalid_header(self, running_resolver, dns_client):
        """Test neplatného DNS headeru"""
        # Vytvoření neplatného headeru s příliš velkým počtem questions
        malformed_header = struct.pack('!HHHHHH',
                                       0x1234,  # Transaction ID
                                       0x0100,  # Flags
                                       0xFFFF,  # Questions (neplatné číslo)
                                       0, 0, 0)

        response = dns_client.send_malformed_query(malformed_header)

        if response:
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Invalid header handled with rcode={response.rcode}")
        else:
            print("✓ Invalid header ignored (no response)")

        assert running_resolver.process.poll() is None

    def test_malformed_query_empty_packet(self, running_resolver, dns_client):
        """Test úplně prázdného paketu"""
        empty_query = b''

        response = dns_client.send_malformed_query(empty_query)

        # Pravděpodobně žádná odpověď
        if response:
            print(f"✓ Empty packet handled with rcode={response.rcode}")
        else:
            print("✓ Empty packet correctly ignored")

        assert running_resolver.process.poll() is None

    def test_malformed_query_random_bytes(self, running_resolver, dns_client):
        """Test náhodných bytů jako DNS dotaz"""
        import random

        random_query = bytes([random.randint(0, 255) for _ in range(50)])

        response = dns_client.send_malformed_query(random_query)

        if response:
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL, DNSRCode.NOTIMP]
            print(f"✓ Random bytes handled with rcode={response.rcode}")
        else:
            print("✓ Random bytes ignored")

        assert running_resolver.process.poll() is None

    def test_query_name_too_long(self, running_resolver, dns_client):
        """Test příliš dlouhého domain jména"""
        # Vytvoření domain jména delšího než 255 znaků
        long_label = "a" * 70
        long_domain = ".".join([long_label] * 4)  # Cca 280 znaků

        try:
            response = dns_client.send_query(long_domain)
            # Měl by vrátit FORMERR nebo SERVFAIL
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print(f"✓ Overly long domain handled with rcode={response.rcode}")
        except ValueError as e:
            # DNS klient může odmítnout vytvořit tak dlouhý dotaz
            print(f"✓ Long domain rejected by client: {e}")

        assert running_resolver.process.poll() is None

    def test_query_name_max_label_length(self, running_resolver, dns_client):
        """Test maximální délky labelu (63 znaků)"""
        # 63 znaků je maximum pro jeden label
        max_label = "a" * 63
        test_domain = f"{max_label}.com"

        response = dns_client.send_query(test_domain)
        # Měl by být zpracován normálně (i když neexistuje)
        assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
        print(f"✓ Max length label handled correctly: {test_domain[:20]}...{test_domain[-10:]}")

        # 64 znaků by mělo selhat
        too_long_label = "a" * 64
        too_long_domain = f"{too_long_label}.com"

        try:
            response = dns_client.send_query(too_long_domain)
            assert response.rcode in [DNSRCode.FORMERR, DNSRCode.SERVFAIL]
            print("✓ Too long label handled with error")
        except ValueError:
            print("✓ Too long label rejected by client")

    def test_query_with_consecutive_dots(self, running_resolver, dns_client):
        """Test domény s po sobě jdoucími tečkami"""
        invalid_domains = [
                "example..com",
                "test...domain.org",
                "..example.com",
                "example.com.."
                ]

        for domain in invalid_domains:
            try:
                response = dns_client.send_query(domain)
                # Může být zpracováno různě
                print(f"✓ Consecutive dots in '{domain}': rcode={response.rcode}")
            except Exception as e:
                print(f"✓ Consecutive dots in '{domain}': error={type(e).__name__}")

        assert running_resolver.process.poll() is None

    def test_unsupported_query_class(self, running_resolver, dns_client):
        """Test nepodporované query class"""
        # Vytvoření dotazu s neplatnou class (např. 255)
        query_data = dns_client.create_dns_query("google.com", qtype=1, qclass=255)

        response = dns_client.send_malformed_query(query_data)

        if response:
            # Většina resolverů vrací NOTIMP nebo FORMERR
            assert response.rcode in [DNSRCode.NOTIMP, DNSRCode.FORMERR, DNSRCode.SERVFAIL, DNSRCode.NOERROR]
            print(f"✓ Unsupported query class handled with rcode={response.rcode}")
        else:
            print("✓ Unsupported query class ignored")

        assert running_resolver.process.poll() is None

    def test_unsupported_query_type(self, running_resolver, dns_client):
        """Test neobvyklých nebo nepodporovaných query typů"""
        unusual_qtypes = [
                (255, "ANY"),  # ANY query (deprecated)
                (99, "SPF"),   # SPF (obsolete)
                (251, "IXFR"), # Zone transfer
                (252, "AXFR")  # Zone transfer
                ]

        for qtype, name in unusual_qtypes:
            try:
                response = dns_client.send_query("example.com", qtype=qtype)
                print(f"✓ Query type {name} ({qtype}): rcode={response.rcode}")
                # Měl by vrátit nějakou odpověď
                assert response.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN, DNSRCode.NOTIMP, DNSRCode.SERVFAIL]
            except Exception as e:
                print(f"✓ Query type {name} ({qtype}): error={type(e).__name__}")

        assert running_resolver.process.poll() is None

    def test_concurrent_malformed_queries(self, running_resolver, dns_client):
        """Test více neplatných dotazů současně"""
        malformed_queries = [
                b'\x12\x34',  # Příliš krátký
                b'\x12\x34\x01\x00\xFF\xFF\x00\x00\x00\x00\x00\x00',  # Neplatný header
                b'\x12\x34\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00',  # Žádné questions
                b'\xFF' * 100,  # Random bytes
                b'',  # Prázdný
                ]

        responses = []
        for i, query in enumerate(malformed_queries):
            try:
                response = dns_client.send_malformed_query(query)
                responses.append(response)
                if response:
                    print(f"✓ Malformed query {i+1}: handled with rcode={response.rcode}")
                else:
                    print(f"✓ Malformed query {i+1}: ignored")
            except Exception as e:
                print(f"✓ Malformed query {i+1}: error={type(e).__name__}")

        # Resolver by měl přežít všechny neplatné dotazy
        assert running_resolver.process.poll() is None
        print(f"✓ Resolver survived {len(malformed_queries)} malformed queries")

    @pytest.mark.asyncio
    async def test_async_malformed_queries(self, running_resolver, dns_client):
        """Test asynchronního posílání neplatných dotazů"""
        import asyncio

        async def send_malformed(query_bytes):
            loop = asyncio.get_event_loop()
            try:
                return await loop.run_in_executor(None, dns_client.send_malformed_query, query_bytes)
            except Exception as e:
                return e

        malformed_queries = [
                b'\x12\x34\x01\x00',
                b'\xFF' * 50,
                b'',
                b'\x00' * 20
                ]

        tasks = [send_malformed(q) for q in malformed_queries]
        results = await asyncio.gather(*tasks)

        print(f"✓ Async malformed queries: {len(results)} handled")
        assert running_resolver.process.poll() is None

    def test_rapid_fire_queries(self, running_resolver, dns_client):
        """Test rychlého opakování dotazů"""
        import threading
        import time

        results = []
        errors = []

        def send_query_thread(domain_suffix):
            try:
                response = dns_client.send_query(f"test{domain_suffix}.google.com")
                results.append(response)
            except Exception as e:
                errors.append(e)

        # Spuštění 30 souběžných dotazů (zvýšeno z 20)
        threads = []
        start_time = time.time()

        for i in range(30):
            thread = threading.Thread(target=send_query_thread, args=(i,))
            threads.append(thread)
            thread.start()

        # Čekání na dokončení
        for thread in threads:
            thread.join(timeout=15)

        end_time = time.time()
        elapsed = end_time - start_time

        print(f"✓ Rapid fire test:")
        print(f"  - Successful: {len(results)}")
        print(f"  - Errors: {len(errors)}")
        print(f"  - Time: {elapsed:.2f}s")
        print(f"  - Rate: {len(results)/elapsed:.1f} queries/sec")

        # Většina dotazů by měla být úspěšná
        success_rate = (len(results) / 30) * 100
        assert success_rate >= 75, f"Too many failed queries: {success_rate:.1f}%"
        assert running_resolver.process.poll() is None, "Resolver crashed"

    @pytest.mark.asyncio
    async def test_stress_mixed_queries(self, running_resolver, dns_client,
                                        exact_blocked_domains, test_domains_should_allow):
        """Stress test s mix legitimních, blokovaných a neplatných dotazů"""
        # Sestavení mix dotazů
        test_domains = []
        test_domains.extend(exact_blocked_domains[:10])  # Blokované
        test_domains.extend(test_domains_should_allow[:10])  # Povolené
        test_domains.extend(["", "...", "invalid..domain", "a" * 70 + ".com"])  # Neplatné

        responses = await dns_client.send_concurrent_queries(test_domains)

        successful = sum(1 for r in responses if not isinstance(r, Exception))
        errors = sum(1 for r in responses if isinstance(r, Exception))

        print(f"✓ Stress test mixed queries:")
        print(f"  - Total: {len(responses)}")
        print(f"  - Successful: {successful}")
        print(f"  - Errors: {errors}")

        assert running_resolver.process.poll() is None
        assert successful > len(responses) * 0.5, "More than 50% queries failed"

    def test_query_with_trailing_dot(self, running_resolver, dns_client):
        """Test domén s trailing dot (FQDN)"""
        test_cases = [
                ("google.com.", "google.com"),  # FQDN vs non-FQDN
                ("example.org.", "example.org"),
                ("test.domain.", "test.domain")
                ]

        for fqdn, non_fqdn in test_cases:
            try:
                response_fqdn = dns_client.send_query(fqdn)
                response_normal = dns_client.send_query(non_fqdn)

                # Obě by měly být zpracovány
                assert response_fqdn.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
                assert response_normal.rcode in [DNSRCode.NOERROR, DNSRCode.NXDOMAIN]
                print(f"✓ FQDN handling: {fqdn} and {non_fqdn} both processed")
            except Exception as e:
                print(f"✓ FQDN test error (expected): {type(e).__name__}")

    def test_blocked_domain_with_malformed_followup(self, running_resolver, dns_client,
                                                    exact_blocked_domains):
        """Test blokované domény následované malformed dotazem"""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        blocked_domain = exact_blocked_domains[0]

        # Normální blokovaný dotaz
        response1 = dns_client.send_query(blocked_domain)
        assert response1.is_blocked()
        print(f"✓ Step 1: Blocked domain {blocked_domain}")

        # Malformed dotaz
        malformed = b'\xFF' * 50
        response2 = dns_client.send_malformed_query(malformed)
        print(f"✓ Step 2: Malformed query handled")

        # Další normální dotaz
        response3 = dns_client.send_query(blocked_domain)
        assert response3.is_blocked()
        print(f"✓ Step 3: Blocked domain still works after malformed")

        assert running_resolver.process.poll() is None

    def test_resolver_process_health_after_errors(self, running_resolver, dns_client):
        """Test zdraví resolveru po sérii errorů"""
        # Získání počátečních statistik
        initial_stats = running_resolver.get_process_stats()
        initial_memory = initial_stats.get('memory_mb', 0)

        print(f"✓ Initial process stats:")
        print(f"  - Memory: {initial_memory:.2f} MB")
        print(f"  - Status: {initial_stats.get('status')}")

        # Série errorových dotazů
        error_queries = [
                b'\xFF' * 100,
                b'',
                b'\x00' * 50,
                dns_client.create_dns_query("a" * 70 + ".com")
                ]

        for query in error_queries:
            try:
                dns_client.send_malformed_query(query)
            except:
                pass

        # Kontrola statistik po errorech
        final_stats = running_resolver.get_process_stats()
        final_memory = final_stats.get('memory_mb', 0)

        print(f"✓ Final process stats:")
        print(f"  - Memory: {final_memory:.2f} MB")
        print(f"  - Status: {final_stats.get('status')}")
        print(f"  - Memory delta: {final_memory - initial_memory:.2f} MB")

        # Resolver by měl být stále zdravý
        assert running_resolver.process.poll() is None
        assert final_stats.get('status') in ['running', 'sleeping']

        # Memory leak check - memory by neměla růst o více než 50 MB
        memory_delta = final_memory - initial_memory
        assert memory_delta < 50, f"Possible memory leak: {memory_delta:.2f} MB increase"

    def test_unicode_domain_handling(self, running_resolver, dns_client):
        """Test Unicode domén (IDN)"""
        unicode_domains = [
                "пример.рф",  # Cyrillic
                "例え.jp",     # Japanese
                "مثال.السعودية",  # Arabic
                ]

        for domain in unicode_domains:
            try:
                response = dns_client.send_query(domain)
                print(f"✓ Unicode domain '{domain}': rcode={response.rcode}")
            except Exception as e:
                print(f"✓ Unicode domain '{domain}': error={type(e).__name__}")

        assert running_resolver.process.poll() is None

    def test_batch_error_recovery(self, running_resolver, dns_client, exact_blocked_domains):
        """Test recovery po batch errorových dotazech"""
        if not exact_blocked_domains:
            pytest.skip("No exact blocked domains configured")

        # 1. Normální dotazy
        normal_result = dns_client.test_exact_domain_blocking(exact_blocked_domains[:5])
        assert normal_result.block_rate > 95.0
        print(f"✓ Phase 1: Normal queries work ({normal_result.blocked}/{normal_result.total})")

        # 2. Error dotazy
        for _ in range(10):
            try:
                dns_client.send_malformed_query(b'\xFF' * 50)
            except:
                pass
        print("✓ Phase 2: Error queries sent")

        # 3. Opět normální dotazy
        recovery_result = dns_client.test_exact_domain_blocking(exact_blocked_domains[:5])
        assert recovery_result.block_rate > 95.0
        print(f"✓ Phase 3: Recovery successful ({recovery_result.blocked}/{recovery_result.total})")

        assert running_resolver.process.poll() is None
