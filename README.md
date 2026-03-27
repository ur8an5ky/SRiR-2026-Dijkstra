# Systemy/Programowanie Równoległe i Rozproszone (PRiR) - 2026

Repozytorium zawiera projekty realizowane w ramach przedmiotu **Systemy/Programowanie Równoległe i Rozproszone** na studiach magisterskich z Informatyki Stosowanej.

## 👥 Zespół
* Jakub Szydełko - **kubarroo**
* Jakub Urbański - **ur8an5ky**

---

## 📂 Struktura repozytorium

Projekt wykorzystuje architekturę monorepo, dzieląc się na dwa główne zadania oraz zbiór narzędzi pomocniczych:

* `1_MPI/` - Projekt 1: Aplikacja równoległa w C/C++ z wykorzystaniem MPI.
* `2_Distributed/` - Projekt 2: Aplikacja rozproszona (Java RMI / CORBA / PGAS).
* `scripts/` - Skrypty pomocnicze (m.in. Python, wizualizacja grafów, generatory danych testowych).

---

## 🚀 Projekt 1: Aplikacja równoległa MPI

* **Temat:** Drzewa wszystkich najkrótszych ścieżek - algorytm Dijkstry.
* **Literatura:** A. Grama, A. Gupta, *Introduction to Parallel Computing* (par. 10.3).
* **Technologie:** C/C++, MPICH / OpenMPI (opcjonalnie wsparte OpenMP / CUDA).
* **Środowisko docelowe:** Pracownia komputerowa WFiIS.
* **Termin oddania:** Lab. nr 9 (od 29.04.2026).

**Krótki opis:**
Celem projektu jest równoległa implementacja algorytmu Dijkstry wyznaczającego najkrótsze ścieżki z każdego węzła grafu (All-Pairs Shortest Path) do wszystkich innych. Liczba węzłów obliczeniowych (procesów MPI) jest dynamicznie dopasowywana do rozmiaru przetwarzanej macierzy sąsiedztwa grafu.

---

## 🌐 Projekt 2: Aplikacja rozproszona

* *(Szczegóły zostaną uzupełnione w późniejszym terminie)*.

---

## 🛠️ Narzędzia pomocnicze (Tools)

W katalogu `scripts/` znajdują się skrypty ułatwiające pracę nad projektami. Obecnie zawiera m.in.:
* Skrypt w Pythonie (wykorzystujący biblioteki ...) do wizualizacji grafów i ...

---

## ⚙️ Instrukcja uruchamiania i kompilacji

*(Sekcja w budowie - instrukcje dotyczące skryptów uruchomieniowych pojawią się tutaj bliżej zakończenia prac nad projektem).*