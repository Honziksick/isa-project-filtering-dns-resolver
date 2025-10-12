import subprocess
import time
import tempfile
import os
import signal
import psutil
from typing import Optional, List, Dict, Tuple
from pathlib import Path
from dataclasses import dataclass
from enum import Enum


class DomainType(Enum):
    """Typ doménového záznamu"""
    EXACT = "exact"
    WILDCARD = "wildcard"


@dataclass
class FilterStats:
    """Statistiky filter souboru"""
    total_entries: int
    exact_domains: int
    wildcard_patterns: int
    comments: int
    empty_lines: int
    invalid_entries: int
    file_path: str


class DNSResolverManager:
    """Správce DNS resolver procesu s podporou exact domén i wildcardů"""

    def __init__(self, binary_path: str = "../../dns", use_valgrind=False):
        self.binary_path = binary_path
        self.use_valgrind = use_valgrind
        self.process: Optional[subprocess.Popen] = None
        self.filter_file: Optional[str] = None
        self.filter_stats: Optional[FilterStats] = None
        self.port = 15353  # Test port
        self.verbose = False

    @staticmethod
    def is_wildcard_pattern(domain: str) -> bool:
        """Detekuje, zda je doména wildcard vzor"""
        return domain.strip().startswith('*.')

    @staticmethod
    def validate_domain(domain: str) -> Tuple[bool, Optional[str]]:
        """
        Validuje doménu nebo wildcard vzor.

        Returns:
            (is_valid, error_message)
        """
        domain = domain.strip()

        if not domain:
            return False, "Empty domain"

        # Wildcard validace
        if domain.startswith('*.'):
            remaining = domain[2:]
            if not remaining:
                return False, "Wildcard without domain part"
            if remaining.startswith('.'):
                return False, "Wildcard with multiple leading dots"
            domain = remaining

        # Základní doménová validace
        if domain.startswith('.') or domain.endswith('.'):
            return False, "Domain starts or ends with dot"

        if '..' in domain:
            return False, "Domain contains consecutive dots"

        # Kontrola délky
        if len(domain) > 253:
            return False, "Domain too long (max 253 chars)"

        # Kontrola labelů
        labels = domain.split('.')
        for label in labels:
            if not label:
                return False, "Empty label"
            if len(label) > 63:
                return False, f"Label too long: {label}"
            # Základní kontrola znaků (zjednodušená)
            if not all(c.isalnum() or c == '-' for c in label):
                return False, f"Invalid characters in label: {label}"

        return True, None

    def create_filter_file(
            self,
            blocked_domains: List[str],
            categorize: bool = True) -> str:
        """
        Vytvoří dočasný filter soubor s optional kategorizací.

        Args:
            blocked_domains: Seznam domén a wildcard vzorů
            categorize: Zda rozdělit domény podle typu
        """
        fd, path = tempfile.mkstemp(
                suffix='.txt',
                prefix='dns_filter_'
                )

        exact_domains = []
        wildcard_patterns = []
        invalid_entries = []

        # Třídění a validace
        for domain in blocked_domains:
            domain = domain.strip()
            if not domain or domain.startswith('#'):
                continue

            is_valid, error = self.validate_domain(domain)
            if not is_valid:
                invalid_entries.append((domain, error))
                print(f"Warning: Invalid entry '{domain}': {error}")
                continue

            if self.is_wildcard_pattern(domain):
                wildcard_patterns.append(domain)
            else:
                exact_domains.append(domain)

        # Zápis do souboru
        with os.fdopen(fd, 'w') as f:
            f.write("# DNS Filter File\n")
            f.write(f"# Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"# Total entries: {len(exact_domains) + len(wildcard_patterns)}\n")
            f.write(f"# Exact domains: {len(exact_domains)}\n")
            f.write(f"# Wildcard patterns: {len(wildcard_patterns)}\n")

            if invalid_entries:
                f.write(f"# Invalid entries (skipped): {len(invalid_entries)}\n")

            f.write("\n")

            if categorize:
                # Exact domény
                if exact_domains:
                    f.write("# ============================================\n")
                    f.write("# EXACT DOMAINS\n")
                    f.write("# ============================================\n")
                    for domain in sorted(exact_domains):
                        f.write(f"{domain}\n")
                    f.write("\n")

                # Wildcard vzory
                if wildcard_patterns:
                    f.write("# ============================================\n")
                    f.write("# WILDCARD PATTERNS\n")
                    f.write("# ============================================\n")
                    for pattern in sorted(wildcard_patterns):
                        f.write(f"{pattern}\n")
            else:
                # Bez kategorizace
                for domain in blocked_domains:
                    domain = domain.strip()
                    if domain and not domain.startswith('#'):
                        f.write(f"{domain}\n")

        self.filter_file = path

        # Statistiky
        self.filter_stats = FilterStats(
                total_entries=len(exact_domains) + len(wildcard_patterns),
                exact_domains=len(exact_domains),
                wildcard_patterns=len(wildcard_patterns),
                comments=3,  # Header comments
                empty_lines=2 if categorize else 0,
                invalid_entries=len(invalid_entries),
                file_path=path
                )

        # Uložení kopie
        self._save_filter_copy(path, exact_domains, wildcard_patterns)

        return path

    def create_filter_file_from_categories(
            self,
            exact_domains: List[str],
            wildcard_patterns: List[str],
            add_header: bool = True) -> str:
        """
        Vytvoří filter soubor z již rozdělených kategorií.

        Args:
            exact_domains: Seznam exact domén
            wildcard_patterns: Seznam wildcard vzorů
            add_header: Zda přidat informační hlavičku
        """
        all_domains = exact_domains + wildcard_patterns
        return self.create_filter_file(all_domains, categorize=True)

    def _save_filter_copy(
            self,
            temp_path: str,
            exact_domains: List[str] = None,
            wildcard_patterns: List[str] = None) -> None:
        """Uloží kopii filter souboru s metadaty"""
        try:
            copies_dir = Path("test_filter_copies")
            copies_dir.mkdir(exist_ok=True)

            timestamp = time.strftime("%Y%m%d_%H%M%S")
            copy_name = f"dns_filter_{timestamp}_{os.getpid()}.txt"
            copy_path = copies_dir / copy_name

            # Zkopíruj soubor
            import shutil
            shutil.copy2(temp_path, copy_path)

            # Vytvoř metadata soubor
            meta_path = copies_dir / f"{copy_name}.meta"
            with open(meta_path, 'w') as f:
                f.write(f"Timestamp: {timestamp}\n")
                f.write(f"PID: {os.getpid()}\n")
                if exact_domains is not None:
                    f.write(f"Exact domains: {len(exact_domains)}\n")
                if wildcard_patterns is not None:
                    f.write(f"Wildcard patterns: {len(wildcard_patterns)}\n")
                if self.filter_stats:
                    f.write(f"Total entries: {self.filter_stats.total_entries}\n")
                    f.write(f"Invalid entries: {self.filter_stats.invalid_entries}\n")

            print(f"Filter file copy saved: {copy_path}")
            print(f"  - Exact domains: {len(exact_domains) if exact_domains else 0}")
            print(f"  - Wildcards: {len(wildcard_patterns) if wildcard_patterns else 0}")

            self._cleanup_old_copies(copies_dir, days=7)

        except Exception as e:
            print(f"Warning: Failed to save filter file copy: {e}")

    def _cleanup_old_copies(
            self,
            copies_dir: Path,
            days: int = 7) -> None:
        """Vymaže staré kopie filter souborů a metadata"""
        try:
            cutoff_time = time.time() - (days * 24 * 3600)

            for file_path in copies_dir.glob("dns_filter_*"):
                if file_path.stat().st_mtime < cutoff_time:
                    file_path.unlink()
                    print(f"Removed old file: {file_path.name}")

        except Exception as e:
            print(f"Warning: Failed to cleanup old copies: {e}")

    def _parse_filter_stats(self, filter_file: str) -> None:
        """Parsuje statistiky z existujícího filter souboru."""
        try:
            exact_count = 0
            wildcard_count = 0
            comment_count = 0
            empty_count = 0
            invalid_count = 0

            if not os.path.exists(filter_file):
                raise FileNotFoundError(f"Filter file not found for parsing: {filter_file}")

            with open(filter_file, 'r') as f:
                for line in f:
                    line = line.strip()

                    if not line:
                        empty_count += 1
                        continue
                    if line.startswith('#'):
                        comment_count += 1
                        continue

                    is_valid, error = self.validate_domain(line)
                    if is_valid:
                        if self.is_wildcard_pattern(line):
                            wildcard_count += 1
                        else:
                            exact_count += 1
                    else:
                        invalid_count += 1
                        print(f"Warning: Invalid entry in '{os.path.basename(filter_file)}': '{line}' - {error}")

            self.filter_stats = FilterStats(
                    total_entries=exact_count + wildcard_count,
                    exact_domains=exact_count,
                    wildcard_patterns=wildcard_count,
                    comments=comment_count,
                    empty_lines=empty_count,
                    invalid_entries=invalid_count,
                    file_path=filter_file
                    )
            print(f"Parsed stats from '{os.path.basename(filter_file)}': {exact_count} exact, {wildcard_count} wildcards.")

        except Exception as e:
            print(f"Error parsing filter stats: {e}")
            self.filter_stats = FilterStats(0, 0, 0, 0, 0, 0, filter_file)
            raise RuntimeError(f"Failed to parse filter file: {filter_file}") from e


    def start_resolver(self,
                       blocked_domains: List[str] = None,
                       exact_domains: List[str] = None,
                       wildcard_patterns: List[str] = None,
                       upstream_dns: str = "8.8.8.8",
                       port: Optional[int] = None,
                       verbose: bool = False,
                       timeout: int = 10,
                       filter_file_costum: str = None) -> None:
        """
        Spustí DNS resolver.

        Args:
            blocked_domains: Seznam všech domén (exact i wildcard) - legacy
            exact_domains: Seznam exact domén (preferovaný způsob)
            wildcard_patterns: Seznam wildcard vzorů (preferovaný způsob)
            upstream_dns: Upstream DNS server
            port: Port pro DNS resolver
            verbose: Verbose logging
            timeout: Timeout pro start (sekundy)
            filter_file_costum: Cesta k vlastnímu filter souboru (přeskočí generování)
        """
        if not os.path.exists(self.binary_path):
            raise FileNotFoundError(f"DNS resolver binary not found: {self.binary_path}")

        # Určení, jaký způsob zadání domén byl použit
        if exact_domains is not None or wildcard_patterns is not None:
            # Nový způsob - oddělené kategorie
            exact_domains = exact_domains or []
            wildcard_patterns = wildcard_patterns or []
            all_domains = exact_domains + wildcard_patterns
            print(f"Using categorized domains: {len(exact_domains)} exact, {len(wildcard_patterns)} wildcards")
        elif blocked_domains is not None:
            # Legacy způsob - všechny dohromady
            all_domains = blocked_domains
            print(f"Using legacy domain list: {len(all_domains)} total entries")
        else:
            # Default testovací domény
            all_domains = [
                    "blocked-domain.com",
                    "evil.example.org",
                    "malware.test",
                    "*.suspicious.net",
                    "*.ads.tracker.com"
                    ]
            print(f"Using default test domains: {len(all_domains)} entries")

        # Vytvoření filter souboru
        filter_file = None
        if filter_file_costum:
            if not os.path.exists(filter_file_costum):
                raise FileNotFoundError(f"Custom filter file not found: {filter_file_costum}")
            filter_file = filter_file_costum
            self._parse_filter_stats(filter_file_costum)
            print(f"Using custom filter file: {filter_file_costum}")
        else:
            filter_file = self.create_filter_file(all_domains, categorize=True)

        if port:
            self.port = port

        # Sestavení příkazu
        if self.use_valgrind:
            cmd = [
                    "valgrind",
                    self.binary_path,
                    "-p", str(self.port),
                    "-f", filter_file,
                    "-s", upstream_dns,
                    "--leak-check=full",
                    "--show-leak-kinds=all",
                    "--track-origins=yes"
                    ]
        else:
            cmd = [
                    self.binary_path,
                    "-p", str(self.port),
                    "-f", filter_file,
                    "-s", upstream_dns
                    ]

        if verbose:
            cmd.append("-v")
            self.verbose = True

        print(f"Starting DNS resolver: {' '.join(cmd)}")
        if self.filter_stats:
            print(f"Filter stats: {self.filter_stats.exact_domains} exact, "
                  f"{self.filter_stats.wildcard_patterns} wildcards, "
                  f"{self.filter_stats.invalid_entries} invalid")

        # Spuštění procesu
        self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE if not verbose else None,
                stderr=None,
                preexec_fn=os.setsid,
                cwd=os.path.dirname(os.path.abspath(self.binary_path))
                )

        # Čekání na start
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.process.poll() is not None:
                stdout, stderr = self.process.communicate()
                raise RuntimeError(
                        f"DNS resolver failed to start.\n"
                        f"Exit code: {self.process.returncode}\n"
                        f"Stdout: {stdout.decode() if stdout else 'None'}\n"
                        f"Stderr: {stderr.decode() if stderr else 'None'}"
                        )

            if self._test_connection():
                print(f"DNS resolver started successfully on port {self.port}")
                return

            time.sleep(0.5)

        # Timeout
        self.stop_resolver()
        raise TimeoutError(f"DNS resolver failed to start within {timeout} seconds")

    def _test_connection(self) -> bool:
        """Test připojení a funkčnosti DNS resolveru"""
        import socket
        import dns.message
        try:
            query = dns.message.make_query('conne.com', 'A')
            wire = query.to_wire()
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(1)
            sock.sendto(wire, ("127.0.0.1", self.port))
            data, _ = sock.recvfrom(4096)
            sock.close()
            return len(data) > 0
        except Exception as e:
            if self.verbose:
                print(f"Test connection failed: {e}")
            return False

    def stop_resolver(self) -> None:
        """Ukončí DNS resolver"""
        if self.process:
            try:
                os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)

                try:
                    self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(os.getpgid(self.process.pid), signal.SIGKILL)
                    self.process.wait()

                print("DNS resolver stopped")

            except ProcessLookupError:
                pass
            except Exception as e:
                print(f"Error stopping resolver: {e}")

            self.process = None

        # Cleanup filter file
        if self.filter_file and os.path.exists(self.filter_file):
            os.unlink(self.filter_file)
            self.filter_file = None

        self.filter_stats = None

    def get_process_stats(self) -> Dict:
        """Získá statistiky procesu"""
        if not self.process:
            return {}

        try:
            proc = psutil.Process(self.process.pid)
            stats = {
                    'cpu_percent': proc.cpu_percent(),
                    'memory_mb': proc.memory_info().rss / 1024 / 1024,
                    'num_threads': proc.num_threads(),
                    'status': proc.status(),
                    'pid': self.process.pid
                    }

            # Přidání filter statistik
            if self.filter_stats:
                stats['filter'] = {
                        'total_entries': self.filter_stats.total_entries,
                        'exact_domains': self.filter_stats.exact_domains,
                        'wildcard_patterns': self.filter_stats.wildcard_patterns,
                        'invalid_entries': self.filter_stats.invalid_entries
                        }

            return stats

        except psutil.NoSuchProcess:
            return {}

    def get_filter_stats(self) -> Optional[FilterStats]:
        """Vrátí statistiky aktuálního filter souboru"""
        return self.filter_stats

    def reload_filter(
            self,
            exact_domains: List[str] = None,
            wildcard_patterns: List[str] = None) -> None:
        """
        Reload filter souboru (restart resolveru s novým filtrem).

        Args:
            exact_domains: Nový seznam exact domén
            wildcard_patterns: Nový seznam wildcard vzorů
        """
        if not self.process:
            raise RuntimeError("Resolver is not running")

        # Uložení aktuální konfigurace
        current_port = self.port
        current_verbose = self.verbose

        # Získání upstream DNS z procesu (zjednodušeno - v reálném případě by se mělo ukládat)
        upstream_dns = "8.8.8.8"

        # Stop a restart
        print("Reloading filter...")
        self.stop_resolver()
        self.start_resolver(
                exact_domains=exact_domains,
                wildcard_patterns=wildcard_patterns,
                port=current_port,
                verbose=current_verbose,
                upstream_dns=upstream_dns
                )
        print("Filter reloaded successfully")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop_resolver()
