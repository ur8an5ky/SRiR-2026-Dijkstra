"""
fake_runner.py - drop-in zastępca DijkstraMPI dla rozwoju GUI.
Czyta macierz w formacie kolegi (TSV, <=0 = brak krawędzi), liczy
sekwencyjny Dijkstra, pluje JSON-em w docelowym formacie.

Użycie:
    python fake_runner.py <matrix_file> --output result.json [--source 0]
"""
import argparse
import json
import time
import heapq
from pathlib import Path


def load_matrix(filepath):
    """Wczytuje macierz sąsiedztwa w formacie TSV (zgodnie z kodem kolegi)."""
    matrix = []
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            row = [int(x) for x in line.split('\t')]
            matrix.append(row)

    n = len(matrix)
    for i, row in enumerate(matrix):
        if len(row) != n:
            raise ValueError(f"Wiersz {i} ma {len(row)} kolumn, oczekiwano {n}")
    return matrix, n


def dijkstra(matrix, n, source):
    """Klasyczny Dijkstra z kolejką priorytetową. Zwraca (dist, parent)."""
    dist = [float('inf')] * n
    parent = [-1] * n
    dist[source] = 0
    visited = [False] * n
    pq = [(0, source)]

    while pq:
        d, u = heapq.heappop(pq)
        if visited[u]:
            continue
        visited[u] = True

        for v in range(n):
            w = matrix[u][v]
            # konwencja kolegi: <=0
            if w <= 0 or v == u:
                continue
            if dist[u] + w < dist[v]:
                dist[v] = dist[u] + w
                parent[v] = u
                heapq.heappush(pq, (dist[v], v))

    return dist, parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix_file")
    parser.add_argument("--source", type=int, default=0)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    t0 = time.perf_counter()
    matrix, n = load_matrix(args.matrix_file)
    t1 = time.perf_counter()

    dist, parent = dijkstra(matrix, n, args.source)
    t2 = time.perf_counter()

    # konwencja JSON: nieosiągalne -> -1
    dist_out = [-1 if d == float('inf') else d for d in dist]

    result = {
        "n": n,
        "source": args.source,
        "num_processes": 1,         # fake = sekwencyjny
        "elapsed_seconds": t2 - t0,
        "io_seconds": t1 - t0,
        "compute_seconds": t2 - t1,
        "dist": dist_out,
        "parent": parent,
    }

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, 'w') as f:
        json.dump(result, f)

    print(f"OK: n={n}, compute={t2-t1:.4f}s, output={args.output}")


if __name__ == "__main__":
    main()