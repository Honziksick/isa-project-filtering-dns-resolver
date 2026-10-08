# Semestrální projekt ISA – Filtrující DNS resolver

**Autor:** Jan Kalina (`xkalinj00`) <br>
**Zadavatel:** Ing. Libor Polčák, Ph.D.

**Univerzita:** Vysoké učení technické v Brně, Fakulta informačních technologií<br>
**Předmět:** *ISA – Síťové aplikace a správa sítí* <br>
**Akademický rok:** *2025/2026*

---

## Licence

Projekt je licencován pod **GNU GPLv3**.

## Podporované platformy

Program funguje na operačních systémech:
- **GNU/Linux** (testováno na Ubuntu 24.04.3 LTS)
- **BSD** systémy (testováno na FreeBSD 14.3-STABLE)

## Hodnocení

**Hodnocení:** 20.0/20.0 b

**Komentář:**

- Věty jako *"Bohužel ve Wiresahrku nejsou pakety viditělné, jelikož z nějakého mně nejasného důvodu odmítá zachytávat komunikaci mezi klientem a mým programem."* vypovídají o upřímnosti, ale do budoucna bych doporučil se nad problémem zamyslet a vyřešit.
- Nevhodná velikost písma v obrázcích v dokumentaci.
- Nevhodné nicneříkající výpisy v dokumentaci, např. sekce 5.
- Modulární kód, členění do funkcí by mohlo být lepší, např. `forwardQueryToResolver()`.

**Výsledek testování:**

- S pomocí programu se podařilo úspěšně překládat doménová jména
- Podařilo úspěšně filtrovat domény
- Program se úspěšně vypořádal s chybějící odpovědí bez filtrace
- Program se úspěšně vypořádal s chybějící odpovědí s filtrací
- Program se úspěšně vypořádal s promíchanými odpověďmi bez filtrace
- Program se úspěšně vypořádal s promíchanými odpověďmi s filtrací
- Program úspěšně filtruje subdomény
- Úspěšně přeložen 1 dotaz
- Úspěšně přeloženy 4 dotazy
- Úspěšně přeložen 1 nefiltrovaný dotaz
- Úspěšně přeloženy 4 nefiltrované dotazy
- OK_subdomeny
- Dotaz úspěšně odfiltrován
- Dotaz úspěšně odfiltrován
- Dotaz úspěšně odfiltrován
- Dobrá rychlost programu

---

## Popis projektu

Filtrující DNS resolver je síťová aplikace implementovaná v **C++20**, která funguje jako transparentní proxy mezi lokálními klienty a upstream DNS resolverem. Aplikace zpracovává DNS dotazy nad protokolem **UDP** a **IPv4**, přičemž filtruje blokované domény podle pravidel v konfiguračním souboru. Program filtruje DNS dotazy typu A směřující na domény v rámci dodaného seznamu a jejich poddomény.

### Klíčové vlastnosti

- Podpora protokolu **DNS** nad **UDP/IPv4**
- Filtrování domén pomocí přesných záznamů a wildcard vzorů (`*.example.com`)
- Blokace domén i jejich subdomén
- Transparentní forwarding povolených dotazů na upstream resolver
- Korektní zpracování DNS komprese jmen (včetně detekce cyklů a forward pointerů)
- Podpora více dotazů v jedné DNS zprávě (`QDCOUNT >= 1`)
- Vícevláknové zpracování s bezpečnou správou transakcí
- Podrobné logování v režimu verbose (`-v`)

### Omezení

- Podporovány jsou pouze dotazy s `QTYPE=1` (A záznamy) a `QCLASS=1` (IN třída)
- Ostatní typy dotazů (AAAA, MX, TXT, CNAME, ...) nejsou podporovány a vedou na `RCODE=NOTIMP`
- Maximální velikost DNS zprávy je 512 bajtů (bez EDNS(0))
- Není podporováno DNSSEC.

---

## Sestavení projektu

```bash
make
```

nebo explicitně:

```bash
make build
```

Pro zobrazení všech dostupných příkazů:

```bash
make help
```

---

## Spuštění programu

### Základní syntaxe

```bash
./dns -s <server> [-p <port>] -f <filter_file> [-v]
```

### Parametry

- `-h, --help` – Zobrazí nápovědu a ukončí program
- `-s, --server` – **Povinný**. Adresa upstream resolveru (hostname nebo IPv4/IPv6)
- `-p, --port` – Volitelný. Cílový port upstream resolveru (výchozí: `53`)
- `-f, --filter` – **Povinný**. Cesta k filtrovacímu souboru s blokovanými doménami
- `-v, --verbose` – Volitelný. Zapne podrobné logování na `STDERR`

### Příklady použití

```bash
# Základní spuštění s veřejným resolverem
./dns -s 8.8.8.8 -f filter.txt

# Spuštění s vlastním portem a verbose režimem
./dns -s 1.1.1.1 -p 5300 -f blocked_domains.txt -v

# Použití hostname místo IP adresy
./dns -s dns.google -f filter.txt
```

---

## Formát filtrovacího souboru

Soubor obsahuje jeden záznam na řádek:

```text
# Komentář začíná '#' a je ignorován
blocked-domain.com          # Blokuje doménu a všechny subdomény
example.org                 # Case-insensitive

# Wildcard vzory blokují pouze subdomény
*.ads-network.com           # Blokuje sub.ads-network.com, ale ne ads-network.com
*.tracking.example          # Blokuje jakoukoliv subdoménu

# Prázdné řádky jsou ignorovány
```

---

## Testování

Projekt obsahuje automatizované integrační testy pomocí **pytest**:

```bash
make test
```

Pro manuální spuštění konkrétního testu:

```bash
cd test
myenv/bin/python -m pytest --tb=short -v IntegrationTests/BasicFunctionalityTest.py
```

### Výsledky testování

Všechny integrační testy úspěšně prošly:
- `BasicFunctionalityTest.py` – 32/32 testů **✓**
- `ComplexFunctionalityTest.py` – 6/6 testů **✓**
- `ErrorHandlingTest.py` – 15/15 testů **✓**
- `CompressedNamesTest.py` – 13/13 testů **✓**

---

## Seznam odevzdaných souborů

```
project-root
├── manual.pdf                      # Dokumentace projektu
├── README.md                       # Tento soubor
├── Makefile                        # Build systém pro GNU Make
├── Makefile.bsd                    # Varianta pro BSD systémy
├── Makefile.gnu                    # Varianta pro GNU systémy
├── CMakeLists.txt                  # CMake konfigurace
├── Doxyfile                        # Konfigurace Doxygen dokumentace
├── src/                            # Zdrojové kódy aplikace
│   ├── App/                        # Hlavní vstupní bod (main.cpp)
│   ├── Arguments/                  # Parsování argumentů příkazové řádky
│   ├── Constants/                  # Konstanty a konfigurační hodnoty
│   ├── DnsUtils/                   # DNS parser, forwarder, messenger
│   ├── Enums/                      # Výčtové typy (RCODE, OPCODE, ...)
│   ├── Exceptions/                 # Hierarchie výjimek
│   ├── Facades/                    # Fasády (MainAppFacade)
│   ├── Filter/                     # Filtrování domén
│   ├── HostnameResolution/         # Resoluce hostname na IP
│   ├── Networking/                 # UDP FSM a socket management
│   └── Utilities/                  # Pomocné utility (logger, signály, ...)
└── test/                           # Testovací framework a skripty
    ├── IntegrationTests/           # Python pytest testy
    ├── filter.txt                  # Ukázkový filtr
    └── run.sh                      # Skript pro spuštění testů
```

---

## Architektura programu ve zkratce

Aplikace je rozdělena do modulárních komponent s jasně vymezenými odpovědnostmi:

- **UdpFsm** – Stavový automat pro příjem a zpracování UDP datagramů
- **DnsMessageParser** – Parser a validátor DNS zpráv včetně komprese jmen
- **DomainFilter** – Vyhodnocování blokovacích pravidel
- **DnsForwarder** – Transparentní forwarding na upstream resolver
- **DnsMessenger** – Generování lokálních odpovědí (REFUSED, FORMERR)

Více informací o implementaci naleznete v **manual.pdf**.
