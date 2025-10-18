################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         conftest.py                                                    #
# Author:       ChatGPT 5 + Jan Kalina <xkalinj00>                             #
#                                                                              #
# Created:      02.10.2025                                                     #
# Last edit:    18.10.2025                                                     #
#                                                                              #
# Description:  Pytest configuration and fixtures for integration tests of     #
#               the Filtering DNS Resolver. Provides reusable domain lists,    #
#               wildcard patterns, resolver process management, test client,   #
#               and temporary filter file utilities. Enables flexible and      #
#               robust test setup for all integration test modules.            #
#                                                                              #
# Please note:  These tests and project testing framework in general were      #
#               developed with the assistance of ChatGPT 5 by OpenAI. Thus,    #
#               please don't consider this code as a subject for plagiarism    #
#               testing.                                                       #
#                                                                              #
################################################################################

import pytest
import os
import tempfile
from ResolverManager import DNSResolverManager
from DnsClient import DNSTestClient


# =============================================================================
# Pytest CLI options
# =============================================================================

def pytest_addoption(parser):
    """Add custom command-line options for the test suite."""
    parser.addoption(
            "--resolver-binary",
            action="store",
            default="../../dns",
            help="Path to the DNS resolver binary",
            )
    parser.addoption(
            "--upstream-dns",
            action="store",
            default="8.8.8.8",
            help="Upstream DNS server address used by tests",
            )
    parser.addoption(
            "--test-port",
            action="store",
            type=int,
            default=15353,
            help="Port to bind the test DNS resolver",
            )
    parser.addoption(
            "--enable-wildcard-tests",
            action="store_true",
            default=True,
            help="Enable wildcard-domain tests",
            )
    parser.addoption(
            "--valgrind",
            action="store_true",
            default=False,
            help="Run the DNS resolver under Valgrind to detect memory leaks",
            )


# =============================================================================
# Session-scoped config fixtures
# =============================================================================

@pytest.fixture(scope="session")
def use_valgrind(request):
    """Whether to run the resolver under Valgrind."""
    return request.config.getoption("--valgrind")


@pytest.fixture(scope="session")
def resolver_binary(request):
    """Absolute path to the resolver binary; skip tests if missing or not executable."""
    binary_path = request.config.getoption("--resolver-binary")

    if not os.path.isabs(binary_path):
        binary_path = os.path.join(os.path.dirname(__file__), binary_path)

    binary_path = os.path.abspath(binary_path)

    if not os.path.exists(binary_path):
        pytest.skip(f"DNS resolver binary not found: {binary_path}")
    if not os.access(binary_path, os.X_OK):
        pytest.skip(f"DNS resolver binary is not executable: {binary_path}")

    return binary_path


@pytest.fixture(scope="session")
def upstream_dns(request):
    """IP of the upstream DNS server used by the resolver."""
    return request.config.getoption("--upstream-dns")


@pytest.fixture(scope="session")
def test_port(request):
    """TCP/UDP port for the test resolver."""
    return request.config.getoption("--test-port")


@pytest.fixture(scope="session")
def enable_wildcard_tests(request):
    """Boolean flag to enable/disable wildcard tests."""
    return request.config.getoption("--enable-wildcard-tests")


# =============================================================================
# Domain lists & patterns
# =============================================================================

@pytest.fixture(scope="session")
def exact_blocked_domains():
    """Exact blocked domains (no wildcards)."""
    return [
            # Core blocked domains
            "blocked-domain.com",
            "evil.example.org",
            "malware.test",

            # Spam domains
            "spam.bad-site.net",
            "suspicious.net",

            # Malware families
            "malware-family.org",
            "trojan.malware.net",

            # Tracking domains
            "tracker.ads.com",
            "analytics.spy.net",

            # Adult content
            "explicit.xxx",

            # Phishing
            "fake-bank.phishing.test",
            "scam-payment.fraud.org",
            ]


@pytest.fixture(scope="session")
def wildcard_patterns():
    """Wildcard patterns to block entire domains and subdomains."""
    return [
            # Block all subdomains of ad servers
            "*.doubleclick.net",
            "*.googlesyndication.com",
            "*.googleadservices.com",

            # Block all tracking domains
            "*.tracking.com",
            "*.analytics-provider.net",

            # Block known malware domains
            "*.malware-distribution.org",
            "*.exploit-kit.ru",

            # Block CDNs serving malicious content
            "*.malicious-cdn.com",

            # Gambling sites
            "*.casino-spam.bet",
            "*.gambling-ads.win",
            ]


@pytest.fixture(scope="session")
def default_blocked_domains(exact_blocked_domains):
    """Backward compatibility: return only the exact domains."""
    return exact_blocked_domains


@pytest.fixture(scope="session")
def combined_filter_domains(exact_blocked_domains, wildcard_patterns):
    """Combined filter set (exact + wildcard)."""
    return {
            "exact": exact_blocked_domains,
            "wildcards": wildcard_patterns,
            }


@pytest.fixture(scope="session")
def test_domains_should_block():
    """Domains that SHOULD be blocked (positive tests)."""
    return {
            # Exact matches
            "exact": [
                    "blocked-domain.com",
                    "malware.test",
                    "spam.bad-site.net",
                    ],
            # Subdomains of exact-blocked domains
            "subdomains": [
                    "sub.blocked-domain.com",
                    "deep.sub.malware.test",
                    "www.spam.bad-site.net",
                    ],
            # Wildcard matches
            "wildcards": [
                    "ads.doubleclick.net",
                    "sub.ads.doubleclick.net",
                    "tracker.analytics-provider.net",
                    "cdn1.malicious-cdn.com",
                    ],
            }


@pytest.fixture(scope="session")
def test_domains_should_allow():
    """Domains that SHOULD NOT be blocked (negative tests)."""
    return [
            # Common legitimate domains
            "google.com",
            "github.com",
            "stackoverflow.com",

            # Similar name, different TLD
            "blocked-domain.org",  # vs blocked-domain.com

            # Contain the blocked string but are not subdomains
            "notblocked-domain.com",
            "blocked-domaintest.com",

            # Wildcard should not match the parent (apex) domain
            "doubleclick.net",  # '*.doubleclick.net' should not match the apex
            "tracking.com",     # '*.tracking.com' should not match the apex
            ]


@pytest.fixture(scope="session")
def edge_case_domains():
    """Edge-case domains for boundary and robustness testing."""
    return {
            "empty": "",
            "single_char": "a",
            "only_tld": ".com",
            "multiple_dots": "...com",
            "trailing_dot": "example.com.",
            "uppercase": "BLOCKED-DOMAIN.COM",
            "mixed_case": "BlOcKeD-DoMaIn.CoM",
            "very_long": "a" * 253 + ".com",  # Intended 'very long' name for boundary tests
            "unicode": "блокированный.домен",
            "idn": "xn--d1acj3b.xn--d1aqf",  # Punycode for блокированный.домен
            }


# =============================================================================
# Resolver manager & running resolver fixtures
# =============================================================================

@pytest.fixture(scope="session")
def resolver_manager(resolver_binary, use_valgrind):
    """Create and manage the lifecycle of the DNS resolver process."""
    manager = DNSResolverManager(resolver_binary, use_valgrind=use_valgrind)
    yield manager
    manager.stop_resolver()


@pytest.fixture(scope="session")
def running_resolver(
        resolver_manager,
        exact_blocked_domains,
        wildcard_patterns,
        upstream_dns,
        test_port,
        ):
    """Run the DNS resolver with both exact domains and wildcard patterns."""
    # Merge exact domains and wildcard patterns into a single list
    all_blocked_entries = exact_blocked_domains + wildcard_patterns

    resolver_manager.start_resolver(
            blocked_domains=all_blocked_entries,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True,
            )
    yield resolver_manager


@pytest.fixture(scope="session")
def running_resolver_exact_only(
        resolver_manager,
        exact_blocked_domains,
        upstream_dns,
        test_port,
        ):
    """Run the DNS resolver with exact domains only (no wildcards)."""
    resolver_manager.start_resolver(
            blocked_domains=exact_blocked_domains,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True,
            )
    yield resolver_manager


@pytest.fixture(scope="session")
def running_resolver_wildcards_only(
        resolver_manager,
        wildcard_patterns,
        upstream_dns,
        test_port,
        ):
    """Run the DNS resolver with wildcard patterns only."""
    resolver_manager.start_resolver(
            blocked_domains=wildcard_patterns,
            upstream_dns=upstream_dns,
            port=test_port,
            verbose=True,
            )
    yield resolver_manager


# =============================================================================
# Client & temp-file fixtures
# =============================================================================

@pytest.fixture(scope="session")
def dns_client(test_port):
    """DNS test client bound to the test resolver port."""
    return DNSTestClient(server_port=test_port)


@pytest.fixture(scope="session")
def temp_filter_file():
    """Temporary filter-file path; removed after the test session."""
    fd, path = tempfile.mkstemp(suffix=".txt", prefix="filter_")
    os.close(fd)  # Close the raw file descriptor
    yield path
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


@pytest.fixture(scope="session")
def temp_filter_file_with_content(temp_filter_file, exact_blocked_domains, wildcard_patterns):
    """Temporary filter-file pre-populated with exact domains and wildcard patterns."""
    with open(temp_filter_file, "w") as f:
        f.write("# Exact blocked domains\n")
        for domain in exact_blocked_domains:
            f.write(f"{domain}\n")
        f.write("\n# Wildcard patterns\n")
        for pattern in wildcard_patterns:
            f.write(f"{pattern}\n")

    return temp_filter_file

### end of file conftest.py ###
