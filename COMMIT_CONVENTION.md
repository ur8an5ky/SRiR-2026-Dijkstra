# 📝 Konwencja Commitów (Conventional Commits)

W tym projekcie stosujemy standard **Conventional Commits**. Pomaga to utrzymać czystą, czytelną historię repozytorium i od razu informuje, czego dotyczy dana zmiana.

## Struktura commita

Każdy commit powinien mieć następujący format:

`typ(opcjonalny_zakres): krótki opis po angielsku`

> **Uwaga:** Opis piszemy w trybie rozkazującym, z małej litery i nie stawiamy kropki na końcu (np. `add python script`, a nie `Added python script.`).

## 🏷️ Dopuszczalne typy (Prefixy)

* **`feat:`** (Feature) - Dodanie nowej funkcjonalności (np. nowy skrypt, implementacja nowej funkcji w MPI).
* **`fix:`** (Bug Fix) - Naprawienie błędu w kodzie.
* **`refactor:`** (Refactoring) - Zmiana w kodzie, która nie dodaje nowej funkcji ani nie naprawia błędu, ale poprawia strukturę/wydajność.
* **`docs:`** (Documentation) - Zmiany wyłącznie w dokumentacji (np. `README.md`, komentarze w kodzie, schematy blokowe w PDF).
* **`chore:`** (Chore) - Zmiany organizacyjne, konfiguracja repozytorium (np. aktualizacja `.gitignore`, zmiany w `Makefile`, zależności).
* **`test:`** (Test) - Dodanie lub poprawa testów weryfikujących poprawność (np. skrypty testowe dla algorytmu Dijkstry).
* **`style:`** (Style) - Zmiany w formatowaniu kodu (np. poprawa wcięć, usunięcie zbędnych spacji), które nie wpływają na jego logikę.

## ✅ Przykłady dobrego użycia

* `feat(python): add initial graph visualization script`
* `fix(mpi): resolve segmentation fault in matrix distribution`
* `docs: update run instructions in README`
* `chore: configure basic Makefile for MPI project`
* `refactor: extract graph loading logic to a separate function`

## ❌ Przykłady, których UNIKAMY

* `poprawki` (Brak typu, język polski, niejasny cel)
* `fix: Fixed bug in Dijkstra.` (Czas przeszły, duża litera, kropka na końcu)
* `dodano pliki` (Niezgodne z konwencją)