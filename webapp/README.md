# Aplikacja webowa - wizualizator drzew najkrótszych ścieżek

Pomocnicza aplikacja webowa do graficznej prezentacji wyników projektu MPI (`1_MPI/DijkstraMPI`). Renderuje graf, podświetla drzewo najkrótszych ścieżek z wybranego źródła, pozwala kliknięciem wybrać wierzchołek docelowy i odtworzyć całą ścieżkę.

> **Uwaga**: Aplikacja stanowi narzędzie pomocnicze projektu MPI - nie jest częścią wymaganego deliverable. Działa w trybie deweloperskim Flaska, bez uwierzytelnienia.

## Funkcjonalność

* Wybór macierzy z `1_MPI/data/` i wczytanie grafu (układ Kamada-Kawai dla małych grafów, ForceAtlas2 client-side dla większych).
* Uruchomienie solvera MPI (`DijkstraMPI`) lub fallbacku `scipy` w procesie Flaska.
* Interaktywne przełączanie wierzchołka źródłowego spośród wszystkich obliczonych drzew - bez ponownego uruchamiania programu.
* Podświetlanie drzewa najkrótszych ścieżek + odtwarzanie wybranej ścieżki kliknięciem w docelowy wierzchołek.
* Statystyki uruchomienia: czas I/O, czas obliczeń, czas całkowity, liczba procesów MPI.

## Wymagania

* Python 3.10+ (testowane na 3.11).
* Dostęp do węzła pracowni `stud204-XX` (dla runnera MPI).
* Zbudowana binarka `1_MPI/build/DijkstraMPI` (jeśli chcemy używać runnera MPI - patrz `1_MPI/README.md`). Bez niej runner MPI w GUI cofa się automatycznie do fallbacku `scipy`.

## Instalacja

Środowisko Pythona można utworzyć przy użyciu **Condy** lub wbudowanego **`venv`** - obie opcje są równoważne, wystarczy wybrać jedną.

### Wariant A - Conda

```bash
conda create -n srir python=3.11 -y
conda activate srir
cd webapp/
pip install -r requirements.txt
```

### Wariant B - `venv`

```bash
cd webapp/
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Uruchomienie

### Krok 1 - konfiguracja MPI i start serwera

W terminalu, w którym aktywne jest środowisko Pythona, na węźle pracowni:

```bash
source /opt/nfs/config/source_mpich500.sh
python3 app.py
```

Flask wystartuje na `http://127.0.0.1:5000`.

### Krok 2 - tunel SSH z własnego urządzenia

Z **lokalnego komputera** (nie z serwera pracowni), w osobnym terminalu, otwórz tunel SSH przez `taurus`:

```bash
ssh -J user@taurus.fis.agh.edu.pl \
    -L 5000:localhost:5000 \
    user@stud204-10
```

Dopóki ta sesja jest aktywna, aplikacja jest dostępna pod `http://127.0.0.1:5000` w przeglądarce na laptopie.

> Można pominąć ten krok, jeśli pracujesz bezpośrednio na maszynie pracowni z dostępem do graficznego pulpitu - wtedy wystarczy odpalić przeglądarkę lokalnie i wejść na `http://127.0.0.1:5000`.

## Architektura

* **Backend** (`app.py`) - Flask, REST API. Uruchamia binarkę MPI w podprocesie i czyta wytworzony JSON. Fallback dla braku MPI: scipy.
* **Frontend** (`templates/index.html`, `static/css/style.css`, `static/js/app.js`) - SPA renderująca graf w `sigma.js` + `graphology`, komunikacja z backendem przez `fetch`.

### REST API

| Endpoint            | Metoda | Opis                                                       |
|---------------------|--------|------------------------------------------------------------|
| `/`                 | GET    | Strona główna (SPA).                                       |
| `/api/graphs`       | GET    | Lista dostępnych macierzy z `1_MPI/data/`.                 |
| `/api/graph`        | POST   | Parsuje wybraną macierz, zwraca wierzchołki/krawędzie/układ. |
| `/api/compute`      | POST   | Uruchamia solver, zwraca wszystkie drzewa najkrótszych ścieżek. |

## Konfiguracja przez zmienne środowiskowe

Domyślne ścieżki można nadpisać:

```bash
export DIJKSTRA_GRAPHS_DIR=/inna/sciezka/do/macierzy
export DIJKSTRA_MPI_BIN=/inna/sciezka/do/DijkstraMPI
export DIJKSTRA_RESULT_PATH=/tmp/wynik.json
python3 app.py
```

## Obsługa - skrót

1. Wybierz plik macierzy z dropdownu **File** → **Load graph**.
2. W sekcji **Solver** wybierz runner (`MPI` lub `in-process`), ustaw **Processes (np)**.
3. **Compute shortest paths** - obliczenia działają dla *wszystkich* źródeł; wynik trafia do JSON-a.
4. Po obliczeniach:
   * **Source node** (dropdown w lewym panelu) - przełącza między drzewami bez recompute'u.
   * Klik w węzeł na grafie - wybiera target, prawy panel pokazuje ścieżkę i jej koszt.
   * Alt+klik (lub Cmd+klik na Macu) - wybiera nowe źródło bez recompute'u.
5. Checkboxy w sekcji **View** włączają/wyłączają etykiety, wagi krawędzi i podświetlanie całego drzewa.