# Projekt ISA: Filtrující DNS resolver

## Hodnocení

- **xx.xx/20.00 b.**
  - Implementace: xx.xx/15.00 b.
  - Dokumentace: xx.xx/5.00 b.

## Společná část zadání projektu ISA

Vytvořte komunikující aplikaci podle konkrétní vybrané specifikace pomocí síťové knihovny BSD sockets (pokud není ve variantě zadání uvedeno jinak). Projekt bude vypracován v jazyce C/C++. Pokud individuální zadání nespecifikuje vlastní referenční systém, musí být projekt přeložitelný a spustitelný na serveru **merlin.fit.vutbr.cz** pod operačním systémem GNU/Linux. Program bude přenositelný. Hodnocení projektů může probíhat na jiném počítači s nainstalovaným OS GNU/Linux, včetně jiných architektur než Intel/AMD, distribucí či verzí knihoven. Pokud vyžadujete minimální verzi knihovny (dostupnou na serveru merlin), jasně tuto skutečnost označte v dokumentaci a README.  
  
### Varianty zadání projektu

- Varianty zadání projektu jsou uveřejněny v systému IS VUT. Přihlašování na varianty bude možné přes IS VUT od 19.9.2025 do 3.10.2025.
- Konzultace k zadání bude probíhat přes fórum v systému Moodle, viz níže.  

### Pokyny k odevzdání projektu

- Vypracovaný projekt uložený v archívu .tar a se jménem **`xlogin00.tar`** odevzdejte elektronicky přes IS VUT. Soubor nekomprimujte.  
- **Termín odevzdání je 17.11.2025** **(hard deadline)**. Odevzdání e-mailem po uplynutí termínu, dodatečné opravy či doplnění kódu nejsou možné.
- **Odevzdaný projekt musí obsahovat:**
  1. soubor se zdrojovým kódem - dodržujte jména souborů uvedená v konkrétním zadání, v záhlaví každého zdrojového souboru by mělo být jméno autora a login,  
  2. funkční _Makefile_ pro překlad zdrojového souboru,
  3. dokumentaci ve formátu PDF (soubor `manual.pdf`), která bude obsahovat uvedení do problematiky, návrhu aplikace, popis implementace, základní informace o programu, návod na použití, popis testování aplikace a výsledky testů. Struktura dokumentace odpovídá technické zprávě a měla by obsahovat následující části: titulní stranu, obsah, logické strukturování textu včetně číslování kapitol, přehled nastudovaných informací z literatury, popis zajímavějších pasáží implementace, použití vytvořených programů a literatura. Pro dokumentaci je možné použít upravenou šablonu pro [bakalářské práce](https://www.fit.vut.cz/study/theses/bachelor-theses/.cs).  
  4. soubor _README_ obsahující jméno a login autora, datum vytvoření, krátký textový popis programu s případnými rozšířeními či omezeními, příklad spuštění a seznam odevzdaných souborů,
  5. další požadované soubory podle konkrétního typu zadání.
- Pokud v projektu nestihnete implementovat všechny požadované vlastnosti, je nutné veškerá omezení jasně uvést v dokumentaci a v souboru README.
- Co není v zadání jednoznačně uvedeno, můžete implementovat podle vlastního uvážení. Zvolené řešení popište v dokumentaci.
- Při řešení projektu respektujte zvyklosti zavedené v OS unixového typu (jako je například formát textového souboru).
- Vytvořené programy by měly být použitelné a smysluplné, řádně okomentované, formátované a členěné do funkcí a modulů. Program by měl obsahovat nápovědu informující uživatele o činnosti programu a očekávaných parametrech. V případě neočekávaného vstupu by měl  vypsat chybové hlášení, případně help.
- Aplikace nesmí v žádném případě skončit s chybou SEGMENTATION FAULT ani jiným násilným systémovým ukončením (např. při dělení nulou).
- Před odevzdáním svůj program důkladně otestujte, viz návod níže. Výsledky experimentů (spuštění aplikace s různými parametry) by měly být součástí dokumentace.
- Pokud přejímáte krátké pasáže zdrojových kódů z různých tutoriálů či příkladů z Internetu (ne mezi sebou), tak je nutné vyznačit zřetelně vyznačit převzaté části v kódu a uvést jejich autory dle autorského zákona vč. uvedení licenčních podmínek, kterými se distribuce daných zdrojových kódů řídí. V případě nedodržení bude na projekt nahlíženo jako na plagiát.
- Za plagiát se považuje i kód, který byl vygenerován externím nástrojem a kde student není autorem kódu ve smyslu autorského zákona. To zahrnuje i různé generátory kódu typu ChatGPT. Student není hodnocen za kód, který vytvořil někdo jiný (osoba či stroj).  
- Konzultace k projektu podává vyučující, který zadání vypsal. Pro své otázky můžete využít diskuzní fórum k projektům.
- Sledujte fórum k projektu, kde se může objevit dovysvětlení či upřesnění zadání.
- Před odevzdáním zkontrolujte, zda projekt obsahuje všechny potřebné soubory a také jste dodrželi jména odevzdávaných souborů pro konkrétní zadání. Zkontrolujte před odevzdáním, zda je projekt přeložitelný na cílové platformě.

### Hodnocení projektů

- Hodnocení projektu bude zveřejněno  v IS VUT.  
- **Maximální počet bodů za projekt je 20 bodů.**
  - Maximálně 15 bodů za plně funkční aplikaci.
  - Maximálně 5 bodů za dokumentaci. Dokumentace se hodnotí pouze v případě funkčního kódu. Pokud kód není odevzdán, nefunguje dle zadání, jedná se o plagiát apod., dokumentace se nehodnotí.
- **Příklad hodnocení projektů:**
  - nepřehledný, nekomentovaný zdrojový text: až -7 bodů
  - nefunkční či chybějící Makefile: až -4 body
  - nekvalitní či chybějící dokumentace: až -5 bodů
  - nedodržení formátu vstupu/výstupu či konfigurace: -10 body
  - odevzdaný soubor nelze přeložit, spustit a odzkoušet: 0 bodů
  - odevzdáno po termínu: 0 bodů
  - nedodržení zadání: 0 bodů
  - nefunkční kód: 0 bodů
  - opsáno: 0 bodů (pro všechny, kdo mají stejný kód), návrh na zahájení disciplinárního řízení.

### Doporučení ověření funkcionality při řešení a před odevzdáním projektu

1. Zapnout a zkontrolovat varování překladače (volba `-Wall`).
2. Ověřit práci s pamětí a striktní využívání inicializovaných proměnných a datových struktur - např. lze využít nástroj _valgrind._
3. Používání debuggeru pro ověření cest programu.
4. Používejte další pomocné nástroje, které vám pomohou zobrazit komunikaci vašich programů. V závislosti na konkrétním zadání se může jednat např. o program Wireshark, dig, openssl a další specializované nástroje.
5. Vytvoření opakovatelných a automatizovaných testů, které ověří, že nový kód nerozbije dříve napsané a ověřené části programu (např. testy jednotkové, integrační a systémové).
6. Časté chyby v odevzdaných projektech zahrnují chybně použité regulární výrazy (často použité zbytečně). U každého regulárního výrazu (regexu) si položte otázky:
    - Je v tomto místě vhodné a účelně regulární výraz použít?
    - Pokud ano, je napsaný regex správně? (Např. v dřívějších letech se objevily regexy, které v URL vyžadovaly jen malá písmenka, zakazovaly některé povolené znaky, předpokládaly doménová jména o určitém počtu částí apod. Takové programy pak na testovaných URL nemusí vůbec fungovat.)
7. Je odevzdaný soubor správně pojmenován?
8. Je odevzdaný soubor správného typu a tedy na referenčním stroji rozbalitelný utilitou tar? (Vizte také _man 1 file_.).
9. Je možné program přeložit programem make?
10. Vytvoří program make správně pojmenovaný soubor v adresáři projektu.  
11. Funguje program na testovacích datech? Nekončí na nějaké chybě, nebo dokonce porušením ochrany paměti?
12. Je v odevzdaném souboru přiložen soubor README a soubor s dokumentací?
13. Jsou soubory s dokumentací, README a spustitelný program (výstup `make`) v kořenovém adresáři projektu, aby jej opravující nemusel hledat v podadresářích?  
14. Jsou všechny soubory správně pojmenované?
15. Testují testy opravdu, že program pracuje správně?
16. Je z dokumentace patrné, že rozumíte teorii, jak jste program navrhli, implementovali, otestovali a jaké byly největší a nejdůležitější problémy, které jste museli vyřešit? Pakliže se od zadání odkloníte (např. implementujete něco navíc, zadání ponechává volnost implementace), je toto patrné z dokumentace?  

## Specifická část zadání: Filtrující DNS resolver

- **Vedoucí:** [Ing. Libor Polčák, Ph.D.](/lide/libor-polcak-78789)

### Popis

- Napište program _dns_, který bude filtrovat dotazy typu A směřující na domény v rámci dodaného seznamu a jejich poddomény. Ostatní dotazy bude přeposílat v nezměněné podobě specifikovanému resolveru. Odpovědi na dříve přeposlané dotazy bude program předávat původnímu tazateli. Analýza a sestavení DNS zpráv musí být implementována přímo v programu _dns_. Stačí uvažovat pouze komunikaci pomocí UDP a dotazy typu A. Na jiné typu dotazů a nežádoucí dotazy odpovídejte vhodnou chybovou zprávou.
- Při vytváření programu je povoleno použít pouze knihovny pro práci se sokety a další obvyklé funkce používané v síťovém prostředí (jako je netinet/\*, sys/\*, arpa/\* apod.), knihovnu pro práci s vlákny (pthread), signály, časem, stejně jako standardní knihovnu jazyka C (varianty ISO/ANSI i POSIX), C++ a STL. Jiné knihovny nejsou povoleny.
  
### Spuštění aplikace

- **Použití:** `dns -s server [-p port] -f filter_file`  
- **Pořadí** parametrů je **libovolné**.
- **Popis parametrů:**
  - **`-s`:** IP adresa nebo doménové jméno DNS serveru (resolveru), kam se má zaslat dotaz.
  - **`-p port`:** Číslo portu, na kterém bude program očekávat dotazy. Výchozí je port 53.
  - **`-f filter_file`:** Jméno souboru obsahující nežádoucí domény.

### Podporované typy dotazů
  
- Uvažujte pouze dotazy typu A, protokol UDP a libovolné protokoly nižších vrstev podporované OS.
- Není požadována podpora DNSSEC.  
  
### Výstup aplikace

- Program nebude vypisovat žádné informace. Volitelně však můžete implementovat parametr -v (verbose), při jehož uvedení bude program vypisovat informace o překladu ve vámi zvoleném formátu.  
  
### Formát souboru se seznamem nežádoucích domén

- Nežádoucí domény budou dopředu uloženy v lokálním textovém `ASCII` souboru, každá doména bude uvedena na samostatném řádku.
- Prázdné řádky a řádky začínající znakem `#` ignorujte.
- Uvažujte konce řádků používané v OS GNU/Linux, Microsoft Windows i Apple Mac OS.  
- **Příklad souboru s nežádoucími doménami:**
  - [https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml&showintro=1](https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml&showintro=1)

### Doplňující informace k zadání

- Před odevzdáním projektu si důkladně pročtěte společné zadání pro všechny projekty.
- Jakékoliv rozšíření nezapomeňte zdůraznit v souboru _README_ a v dokumentaci. Není však možné získat více bodů, než je stanovené maximum.
- Program se musí vypořádat s chybnými vstupy.
- Veškeré chybové výpisy vypisujte srozumitelně na standardní chybový výstup.
- Pokud máte pocit, že v zadání není něco specifikováno, popište v dokumentaci vámi zvolené řešení a zdůvodněte, proč jste jej vybrali.
- V dokumentaci popište, jaké řešení jste zvolili pro procházení seznamu blokovaných domén.
- V dokumentaci uveďte, jaké chybové zprávy váš program generuje a za jakých okolností.
- Vytvořený kód by měl být modulární a otestovaný. Testy, které jste při řešení projektu napsali se spustí voláním `make test`.
- Pište [robustní](https://en.wikipedia.org/wiki/Robustness_principle) aplikace, které budou na vstupu vstřícné k drobným odchylkám od specifikace.
- Při řešení projektu uplatněte znalosti získané v dřívějších kurzech týkající se jak zdrojového kódu (formátování, komentáře), pojmenování souborů, tak vstřícnosti programu k uživateli.

### Referenční prostředí pro překlad a testování

- Program by měl být přenositelný. Referenční prostředí pro překlad budou servery `eva.fit.vutbr.cz` a `merlin.fit.vutbr.cz` (program musí být přeložitelný a funkční na obou systémech).
- Vlastní testování může probíhat na jiném počítači s nainstalovaným OS _GNU/Linux_, či _FreeBSD_, včetně jiných architektur než _Intel/AMD_, jiných distribucí, jiných verzí knihoven apod. Pokud vyžadujete minimální verzi knihovny (dostupné na serveru merlin a eva), jasně tuto skutečnost označte v dokumentaci a _README_.  
  
### Doporučená literatura

- [RFC1035](https://tools.ietf.org/html/rfc1035)
