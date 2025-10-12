import pytest
import os
import sys
import tempfile
from pathlib import Path
from ResolverManager import DNSResolverManager
from DnsClient import DNSTestClient

def pytest_addoption(parser):
    """Přidání custom argumentů pro pytest"""
    parser.addoption(
            "--resolver-binary",
            action="store",
            default="../../dns",
            help="Path to DNS resolver binary"
            )
    parser.addoption(
            "--upstream-dns",
            action="store",
            default="8.8.8.8",
            help="Upstream DNS server for tests"
            )
    parser.addoption(
            "--test-port",
            action="store",
            type=int,
            default=15353,
            help="Port for test DNS resolver"
            )
    parser.addoption(
            "--enable-wildcard-tests",
            action="store_true",
            default=True,
            help="Enable wildcard domain tests"
            )
    parser.addoption(
            "--valgrind",
            action="store_true",
            default=False,
            help="Run DNS resolver under Valgrind for memory leak detection"
            )

@pytest.fixture(scope="session")
def use_valgrind(request):
    """Valgrind enable flag"""
    return request.config.getoption("--valgrind")

@pytest.fixture(scope="session")
def resolver_binary(request):
    """Binary path fixture"""
    binary_path = request.config.getoption("--resolver-binary")

    if not os.path.isabs(binary_path):
        binary_path = os.path.join(
                os.path.dirname(__file__),
                binary_path
                )

    binary_path = os.path.abspath(binary_path)

    if not os.path.exists(binary_path):
        pytest.skip(f"DNS resolver binary not found: {binary_path}")
    if not os.access(binary_path, os.X_OK):
        pytest.skip(f"DNS resolver binary is not executable: {binary_path}")

    return binary_path


@pytest.fixture(scope="session")
def upstream_dns(request):
    """Upstream DNS server fixture"""
    return request.config.getoption("--upstream-dns")


@pytest.fixture(scope="session")
def test_port(request):
    """Test port fixture"""
    return request.config.getoption("--test-port")


@pytest.fixture(scope="session")
def enable_wildcard_tests(request):
    """Wildcard tests enable flag"""
    return request.config.getoption("--enable-wildcard-tests")


@pytest.fixture(scope="session")
def exact_blocked_domains():
    """Přesné blokované domény (bez wildcardů)"""
    return [
            # Základní blokované domény
            "blocked-domain.com",
            "evil.example.org",
            "malware.test",

            # Spam domény
            "spam.bad-site.net",
            "suspicious.net",

            # Malware rodiny
            "malware-family.org",
            "trojan.malware.net",

            # Tracking domény
            "tracker.ads.com",
            "analytics.spy.net",

            # Adult content
            "explicit.xxx",

            # Phishing
            "fake-bank.phishing.test",
            "scam-payment.fraud.org"
            ]


@pytest.fixture(scope="session")
def wildcard_patterns():
    """Wildcard vzory pro blokování celých domén a subdomén"""
    return [
            # Blokovat všechny subdomény ad serverů
            "*.doubleclick.net",
            "*.googlesyndication.com",
            "*.googleadservices.com",

            # Blokovat všechny tracking domény
            "*.tracking.com",
            "*.analytics-provider.net",

            # Blokovat všechny známé malware domény
            "*.malware-distribution.org",
            "*.exploit-kit.ru",

            # Blokovat všechny CDN pro škodlivý obsah
            "*.malicious-cdn.com",

            # Gambling sites
            "*.casino-spam.bet",
            "*.gambling-ads.win"
            ]


@pytest.fixture(scope="session")
def default_blocked_domains(exact_blocked_domains):
    """Zpětná kompatibilita - vrací pouze exact domény"""
    return exact_blocked_domains


@pytest.fixture(scope="session")
def combined_filter_domains(exact_blocked_domains, wildcard_patterns):
    """Kombinované domény (exact + wildcard) pro kompletní filtr"""
    return {
            'exact': exact_blocked_domains,
            'wildcards': wildcard_patterns
            }


@pytest.fixture(scope="session")
def test_domains_should_block():
    """Domény které BY MĚLY být zablokovány (pro pozitivní testy)"""
    return {
            # Exact matches
            'exact': [
                    "blocked-domain.com",
                    "malware.test",
                    "spam.bad-site.net"
                    ],
            # Subdoména blokované exact domény
            'subdomains': [
                    "sub.blocked-domain.com",
                    "deep.sub.malware.test",
                    "www.spam.bad-site.net"
                    ],
            # Wildcard matches
            'wildcards': [
                    "ads.doubleclick.net",
                    "sub.ads.doubleclick.net",
                    "tracker.analytics-provider.net",
                    "cdn1.malicious-cdn.com"
                    ]
            }


@pytest.fixture(scope="session")
def test_domains_should_allow():
    """Domény které BY NEMĚLY být zablokovány (pro negativní testy)"""
    return [
            # Běžné legitimní domény
            "google.com",
            "github.com",
            "stackoverflow.com",

            # Podobné názvy, ale jiná TLD
            "blocked-domain.org",  # vs blocked-domain.com

            # Domény obsahující blokovaný string, ale nejsou subdoménou
            "notblocked-domain.com",
            "blocked-domaintest.com",

            # Wildcard by neměl matchovat parent doménu
            "doubleclick.net",  # *.doubleclick.net by nemělo matchovat samotnou doubleclick.net
            "tracking.com"      # *.tracking.com by nemělo matchovat samotnou tracking.com
            ]


@pytest.fixture(scope="session")
def edge_case_domains():
    """Edge case domény pro testování okrajových případů"""
    return {
            'empty': "",
            'single_char': "a",
            'only_tld': ".com",
            'multiple_dots': "...com",
            'trailing_dot': "example.com.",
            'uppercase': "BLOCKED-DOMAIN.COM",
            'mixed_case': "BlOcKeD-DoMaIn.CoM",
            'very_long': "a" * 253 + ".com",  # Max DNS label je 253 znaků
            'unicode': "блокированный.домен",
            'idn': "xn--d1acj3b.xn--d1aqf",  # punycode pro блокированный.домен
            }


@pytest.fixture(scope="session")
def resolver_manager(resolver_binary, use_valgrind):
    """DNS resolver manager fixture"""
    manager = DNSResolverManager(resolver_binary, use_valgrind=use_valgrind)
    yield manager
    manager.stop_resolver()


@pytest.fixture(scope="session")
def running_resolver(
        resolver_manager,
        exact_blocked_domains,
        wildcard_patterns,
        upstream_dns,
        test_port
        ):
    """Běžící DNS resolver s exact doménami i wildcardy"""
    # Kombinujeme exact domény a wildcard vzory do jednoho seznamu
    all_blocked_entries = exact_blocked_domains + wildcard_patterns

    resolver_manager.start_resolver(
            blocked_domains=all_blocked_entries,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True
            )
    yield resolver_manager


@pytest.fixture(scope="session")
def running_resolver_exact_only(
        resolver_manager,
        exact_blocked_domains,
        upstream_dns,
        test_port
        ):
    """Běžící DNS resolver pouze s exact doménami (bez wildcardů)"""
    resolver_manager.start_resolver(
            blocked_domains=exact_blocked_domains,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True
            )
    yield resolver_manager


@pytest.fixture(scope="session")
def running_resolver_wildcards_only(
        resolver_manager,
        wildcard_patterns,
        upstream_dns,
        test_port
        ):
    """Běžící DNS resolver pouze s wildcard vzory"""
    resolver_manager.start_resolver(
            blocked_domains=wildcard_patterns,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True
            )
    yield resolver_manager


@pytest.fixture(scope="session")
def dns_client(test_port):
    """DNS test client fixture"""
    return DNSTestClient(server_port=test_port)


@pytest.fixture(scope="session")
def temp_filter_file():
    """Dočasný filter soubor fixture"""
    fd, path = tempfile.mkstemp(suffix='.txt', prefix='filter_')
    os.close(fd)  # Zavřeme file descriptor
    yield path
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


@pytest.fixture(scope="session")
def temp_filter_file_with_content(temp_filter_file, exact_blocked_domains, wildcard_patterns):
    """Dočasný filter soubor s připraveným obsahem"""
    with open(temp_filter_file, 'w') as f:
        f.write("# Exact blocked domains\n")
        for domain in exact_blocked_domains:
            f.write(f"{domain}\n")
        f.write("\n# Wildcard patterns\n")
        for pattern in wildcard_patterns:
            f.write(f"{pattern}\n")

    return temp_filter_file
