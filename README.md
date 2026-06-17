# Systemy/Programowanie Równoległe i Rozproszone (PRiR) - 2026

Repozytorium zawiera projekty realizowane w ramach przedmiotu **Systemy/Programowanie Równoległe i Rozproszone** na studiach magisterskich z Informatyki Stosowanej.

## 👥 Zespół
* Jakub Szydełko - **kubarroo**
* Jakub Urbański - **ur8an5ky**

---

## 📂 Struktura repozytorium

Projekt wykorzystuje architekturę monorepo, dzieląc się na dwa główne zadania oraz zbiór narzędzi pomocniczych:

* `1_MPI/` - Projekt 1: Aplikacja równoległa w C++ z wykorzystaniem MPI.
* `2_Distributed/` - Projekt 2: Aplikacja rozproszona w modelu PGAS z wykorzystaniem UPC++.
* `scripts/` - Skrypty pomocnicze (Python): generator losowych grafów spójnych, narzędzia wizualizacyjne, fake_runner do testów.
* `webapp/` - Aplikacja webowa (Flask) do graficznej wizualizacji wyników projektów.

---

## 🚀 Projekt 1: Aplikacja równoległa MPI

* **Temat:** Drzewa wszystkich najkrótszych ścieżek - algorytm Dijkstry.
* **Literatura:** A. Grama, A. Gupta, *Introduction to Parallel Computing* (par. 10.3).
* **Technologie:** C++17, MPICH 5.0.

### Krótki opis

Celem projektu jest równoległa implementacja algorytmu Dijkstry wyznaczającego drzewa najkrótszych ścieżek dla każdego źródła w grafie ważonym (All-Pairs Shortest Path). Implementacja zgodna z formułą **source-parallel** opisaną w rozdziale 10.3 podręcznika Gramy/Gupty - pojedynczy przebieg algorytmu Dijkstry zrównoleglony przez podział wierzchołków pomiędzy procesy MPI.

Liczba procesów MPI jest dynamicznie dopasowywana do rozmiaru przetwarzanej macierzy sąsiedztwa grafu.

### Jak uruchomić

#### Wymagania

* Kompilator C++ z obsługą standardu C++17.
* MPICH 5.0 (dostępne w pracowni przez `/opt/nfs/config/source_mpich500.sh`).
* GNU Make.

#### Krok 1 - połączenie z węzłem klastra

Logowanie dwustopniowe: najpierw na serwer dostępowy `taurus`, następnie na wybraną stację z listy (dla nas to: `stud204-00` ... `stud204-15`):

```bash
ssh user@taurus.fis.agh.edu.pl
ssh stud204-XX
```

#### Krok 2 - konfiguracja środowiska MPI

```bash
source /opt/nfs/config/source_mpich500.sh
```

#### Krok 3 - kompilacja i uruchomienie

```bash
cd 1_MPI/
make            # kompilacja
make run        # uruchomienie z domyślnymi danymi (data/matrix_4.txt, NP=16)
```

Liczbę procesów oraz plik wejściowy można nadpisać:

```bash
make run NP=8
make run NP=16 DATA_FILE=data/matrix_100.txt
```

Sprzątanie:

```bash
make clean
```

### Format danych

**Wejście** (TSV) - kwadratowa macierz sąsiedztwa, wartości oddzielone tabulatorami:
* `-1` - brak krawędzi (w szczególności na przekątnej),
* liczba dodatnia - waga krawędzi.

**Wyjście**:
* Standardowe wyjście - wszystkie najkrótsze ścieżki dla każdego źródła w czytelnym formacie `s -> v: dist | path: ...`.
* Plik JSON - generowany automatycznie obok pliku z macierzą (np. `data/matrix_4.txt` → `data/matrix_4.json`), zawiera kompletny zbiór drzew najkrótszych ścieżek (`dist[s][v]`, `parent[s][v]`) oraz statystyki czasu (I/O, compute, total). Wykorzystywany przez aplikację webową.

### Generowanie macierzy testowych

```bash
cd scripts/
python3 random_matrix_generator.py 25                     # 25 wierzchołków, gęstość 0.4
python3 random_matrix_generator.py 100 --density 0.3      # 100 wierzchołków, gęstość 0.3
python3 random_matrix_generator.py 50 --seed 42           # powtarzalne losowanie
```

Generator gwarantuje spójność grafu - najpierw buduje losowe drzewo rozpinające, następnie dorzuca krawędzie do osiągnięcia zadanej gęstości. Wynik trafia do `1_MPI/data/matrix_<N>.txt`.

### Pełna dokumentacja

Szczegółowy opis algorytmu, zrównoleglenia (formuła source-parallel z Gramy/Gupty), schematów blokowych i implementacji znajduje się w pliku PDF `1_MPI/dokumentacja.pdf`.

---

## 🌐 Projekt 2: Aplikacja rozproszona (PGAS / UPC++)

* **Temat:** Drzewa wszystkich najkrótszych ścieżek - algorytm Dijkstry (ten sam co w Projekcie 1).
* **Literatura:** A. Grama, A. Gupta, *Introduction to Parallel Computing* (par. 10.3).
* **Technologie:** C++17, UPC++ 2025.10 (model PGAS, transport GASNet/UDP).

### Krótki opis

Ten sam algorytm co w Projekcie 1, zaimplementowany w modelu **PGAS** (Partitioned Global Address Space) z wykorzystaniem biblioteki **UPC++**. Zachowano identyczną dekompozycję source-parallel (podział wierzchołków pomiędzy procesy), natomiast warstwa komunikacji jest inna niż w MPI:

* selekcja globalnego minimum przez `upcxx::reduce_all` z własnym operatorem MINLOC (odpowiednik `MPI_Allreduce`),
* **zbieranie wyników przez komunikację jednostronną** (`rput` do globalnej przestrzeni adresowej) - każdy proces zapisuje swój blok wyników wprost do pamięci procesu 0 przez `global_ptr`, bez aktywnego udziału odbiorcy.

To właśnie jednostronny dostęp do zdalnej pamięci (RMA) odróżnia model PGAS od czystego message-passing z Projektu 1. Program produkuje wynik w **tym samym formacie JSON** co wersja MPI, dzięki czemu współpracuje z tą samą aplikacją webową.

### Jak uruchomić

#### Wymagania

* Kompilator C++ z obsługą standardu C++17.
* UPC++ 2025.10 (dostępne w pracowni przez `/opt/nfs/config/source_upcxx.sh`).
* GNU Make.

#### Krok 1 - połączenie z węzłem klastra

```bash
ssh user@taurus.fis.agh.edu.pl
ssh stud204-XX
```

#### Krok 2 - konfiguracja środowiska UPC++

```bash
source /opt/nfs/config/source_upcxx.sh
```

#### Krok 3 - kompilacja i uruchomienie

```bash
cd 2_Distributed/
make            # kompilacja (conduit UDP)
make run        # uruchomienie na klastrze (NP=4, data/matrix_25.txt)
```

Liczbę zadań (*ranks*) oraz plik wejściowy można nadpisać:

```bash
make run NP=8
make run NP=16 DATA_FILE=data/matrix_100.txt
```

Uruchomienie lokalne (jeden host, bez listy węzłów):

```bash
make run-local NP=4 DATA_FILE=data/matrix_4.txt
```

Sprzątanie:

```bash
make clean
```

### Format danych

Identyczny jak w Projekcie 1 (TSV na wejściu, JSON na wyjściu) - obie aplikacje są w pełni kompatybilne i mogą korzystać z tych samych macierzy testowych oraz tej samej wizualizacji.

### Pełna dokumentacja

*(Szczegółowy opis modelu PGAS, mapowania komunikacji MPI → UPC++ oraz implementacji - w pliku PDF `2_Distributed/dokumentacja.pdf`)*.

---

## 🎨 Aplikacja webowa - wizualizacja wyników

Pomocnicza aplikacja Flask udostępnia interaktywną wizualizację drzew najkrótszych ścieżek. Ponieważ oba projekty (MPI i UPC++) produkują wynik w tym samym formacie JSON, aplikacja obsługuje wyniki z obu. Pozwala wczytać dowolną macierz, uruchomić solver lub fallback `scipy`, przełączać wierzchołek źródłowy bez ponownego liczenia oraz inspekcjonować ścieżki kliknięciem.

Pełna instrukcja uruchamiania (Conda lub `venv`, konfiguracja środowiska, tunelowanie portu) - w pliku [`webapp/README.md`](webapp/README.md).

---

## 🛠️ Narzędzia pomocnicze

W katalogu `scripts/`:

* `random_matrix_generator.py` - generator losowych macierzy spójnych grafów ważonych.
* `visualize_graph.py` - prosta wizualizacja grafu w `matplotlib` (sanity-check macierzy).
* `fake_runner.py` - alternatywny solver Pythonowy zwracający wyniki w formacie zgodnym z `DijkstraMPI` (głównie do testów aplikacji webowej, gdy binarka MPI nie jest jeszcze zbudowana).