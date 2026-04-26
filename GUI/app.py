"""
Flask backend for the Dijkstra MPI visualization.

Exposes a small JSON API used by the single-page frontend:
    GET  /                  -> renders the SPA
    GET  /api/graphs        -> list TSV files in GRAPHS_DIR
    POST /api/graph         -> parse a TSV adjacency matrix, return nodes/edges + layout
    POST /api/compute       -> run the solver (MPI binary, fake_runner, or in-process fallback)
    POST /api/benchmark     -> run /api/compute for several np values, return timings

Configuration is via environment variables (see README).
"""

from __future__ import annotations

import json
import os
import subprocess
import time
from pathlib import Path
from typing import Any

import networkx as nx
import numpy as np
from flask import Flask, jsonify, render_template, request
from scipy.sparse import csr_matrix
from scipy.sparse.csgraph import dijkstra

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PROJECT_ROOT = Path(__file__).resolve().parent
GRAPHS_DIR = Path(os.environ.get("DIJKSTRA_GRAPHS_DIR", PROJECT_ROOT.parent / "scripts" / "test_matrices"))
MPI_BINARY = Path(os.environ.get("DIJKSTRA_MPI_BIN", PROJECT_ROOT.parent / "build" / "DijkstraMPI"))
FAKE_RUNNER = Path(os.environ.get("DIJKSTRA_FAKE_RUNNER", PROJECT_ROOT.parent / "scripts" / "fake_runner.py"))
RESULT_PATH = Path(os.environ.get("DIJKSTRA_RESULT_PATH", "/tmp/dijkstra_result.json"))

# Layouts above this node count switch from kamada-kawai to spring (cheaper).
LAYOUT_KAMADA_LIMIT = 200
# Above this we send raw nodes/edges and let the client run forceatlas2.
LAYOUT_CLIENT_LIMIT = 2000

app = Flask(__name__)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _parse_tsv_matrix(path: Path) -> tuple[int, list[tuple[int, int, int]], bool]:
    """Parse a TSV adjacency matrix.

    Values <= 0 are treated as "no edge". The matrix is checked for symmetry;
    if symmetric, only edges with i < j are emitted (undirected). Otherwise
    every nonzero off-diagonal entry becomes a directed edge.

    Returns (n, edges, is_directed).
    """
    rows: list[list[int]] = []
    with open(path) as fh:
        for line in fh:
            line = line.rstrip("\n")
            if not line:
                continue
            rows.append([int(tok) for tok in line.split("\t")])

    n = len(rows)
    if any(len(r) != n for r in rows):
        raise ValueError("Matrix is not square")

    # Detect symmetry while iterating.
    is_directed = False
    for i in range(n):
        for j in range(i + 1, n):
            a = rows[i][j] if rows[i][j] > 0 else 0
            b = rows[j][i] if rows[j][i] > 0 else 0
            if a != b:
                is_directed = True
                break
        if is_directed:
            break

    edges: list[tuple[int, int, int]] = []
    if is_directed:
        for i in range(n):
            for j in range(n):
                if i == j:
                    continue
                w = rows[i][j]
                if w > 0:
                    edges.append((i, j, w))
    else:
        for i in range(n):
            for j in range(i + 1, n):
                w = rows[i][j]
                if w > 0:
                    edges.append((i, j, w))

    return n, edges, is_directed


def _compute_layout(n: int, edges: list[tuple[int, int, int]]) -> dict[int, tuple[float, float]] | None:
    """Compute node positions server-side. Returns None for very large graphs
    (the client will compute the layout instead)."""
    if n > LAYOUT_CLIENT_LIMIT:
        return None

    g = nx.Graph()
    g.add_nodes_from(range(n))
    # networkx's layouts ignore weights for the most part; that's fine here.
    g.add_edges_from((u, v) for u, v, _ in edges)

    if n <= LAYOUT_KAMADA_LIMIT and len(edges) > 0:
        try:
            pos = nx.kamada_kawai_layout(g)
        except Exception:  # disconnected components, etc.
            pos = nx.spring_layout(g, seed=42, iterations=100)
    else:
        pos = nx.spring_layout(g, seed=42, iterations=50)

    # Normalize to roughly [-1, 1] x [-1, 1].
    return {int(k): (float(v[0]), float(v[1])) for k, v in pos.items()}


def _solve_in_process(matrix_path: Path, source: int) -> dict[str, Any]:
    """Fallback solver: run scipy's Dijkstra directly. Same JSON shape as the
    MPI binary should produce."""
    t0 = time.perf_counter()

    rows: list[list[int]] = []
    with open(matrix_path) as fh:
        for line in fh:
            line = line.rstrip("\n")
            if not line:
                continue
            rows.append([int(tok) for tok in line.split("\t")])

    n = len(rows)
    arr = np.zeros((n, n), dtype=np.float64)
    for i in range(n):
        for j in range(n):
            w = rows[i][j]
            arr[i][j] = w if w > 0 and i != j else 0  # 0 means "no edge" in scipy
    t_io = time.perf_counter() - t0

    t1 = time.perf_counter()
    csr = csr_matrix(arr)
    dist, predecessors = dijkstra(csgraph=csr, directed=False, indices=source, return_predecessors=True)
    t_compute = time.perf_counter() - t1

    dist_list: list[int] = []
    for d in dist:
        if np.isinf(d):
            dist_list.append(-1)
        else:
            dist_list.append(int(d))

    parent_list = [int(p) if p >= 0 else -1 for p in predecessors]
    parent_list[source] = -1

    return {
        "n": n,
        "source": source,
        "num_processes": 1,
        "elapsed_seconds": t_io + t_compute,
        "io_seconds": t_io,
        "compute_seconds": t_compute,
        "dist": dist_list,
        "parent": parent_list,
        "runner": "in_process_scipy",
    }


def _solve_via_subprocess(cmd: list[str], runner_label: str) -> dict[str, Any]:
    """Run a subprocess that writes RESULT_PATH, then read it back."""
    if RESULT_PATH.exists():
        RESULT_PATH.unlink()

    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    wall = time.perf_counter() - t0

    if proc.returncode != 0:
        raise RuntimeError(
            f"{runner_label} failed (exit {proc.returncode}).\n"
            f"stdout: {proc.stdout[-500:]}\nstderr: {proc.stderr[-500:]}"
        )

    if not RESULT_PATH.exists():
        raise RuntimeError(f"{runner_label} produced no output at {RESULT_PATH}")

    with open(RESULT_PATH) as fh:
        result = json.load(fh)

    result.setdefault("runner", runner_label)
    result.setdefault("wall_seconds", wall)
    return result


def _select_runner(requested: str | None) -> str:
    """Pick the actual runner to use based on what the user asked for and what
    is available on disk."""
    if requested == "mpi":
        if MPI_BINARY.exists():
            return "mpi"
        return "in_process"  # silently fall back; frontend shows the chosen runner
    if requested == "fake":
        if FAKE_RUNNER.exists():
            return "fake"
        return "in_process"
    return "in_process"


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------


@app.route("/")
def index() -> Any:
    return render_template(
        "index.html",
        runners_available={
            "mpi": MPI_BINARY.exists(),
            "fake": FAKE_RUNNER.exists(),
            "in_process": True,
        },
    )


@app.route("/api/graphs")
def list_graphs() -> Any:
    out = []
    if GRAPHS_DIR.exists():
        for f in sorted(GRAPHS_DIR.iterdir()):
            if f.suffix.lower() not in {".tsv", ".txt", ".dat"}:
                continue
            out.append({
                "name": f.name,
                "size_kb": round(f.stat().st_size / 1024, 1),
            })
    return jsonify(out)


@app.route("/api/graph", methods=["POST"])
def load_graph() -> Any:
    data = request.get_json(force=True)
    filename = data.get("filename")
    if not filename:
        return jsonify({"error": "filename required"}), 400

    path = GRAPHS_DIR / filename
    if not path.exists():
        return jsonify({"error": f"{filename} not found in {GRAPHS_DIR}"}), 404

    try:
        n, edges, is_directed = _parse_tsv_matrix(path)
    except Exception as e:
        return jsonify({"error": f"failed to parse: {e}"}), 400

    layout = _compute_layout(n, edges)

    return jsonify(
        {
            "n": n,
            "edges": edges,
            "directed": is_directed,
            "layout": (
                {str(k): {"x": v[0], "y": v[1]} for k, v in layout.items()}
                if layout is not None
                else None
            ),
            "needs_client_layout": layout is None,
        }
    )


@app.route("/api/compute", methods=["POST"])
def compute() -> Any:
    data = request.get_json(force=True)
    filename = data.get("filename")
    np_count = int(data.get("np", 1))
    requested_runner = data.get("runner")  # "mpi" | "fake" | "in_process" | None

    if not filename:
        return jsonify({"error": "filename required"}), 400

    matrix_path = GRAPHS_DIR / filename
    if not matrix_path.exists():
        return jsonify({"error": f"{filename} not found"}), 404

    runner = _select_runner(requested_runner)

    try:
        if runner == "mpi":
            cmd = [
                "mpirun", "-np", str(np_count), str(MPI_BINARY),
                str(matrix_path),
                # NOTE: these flags are the ones we agreed your teammate will add.
                # Until then, the binary may ignore them and write to stdout instead.
                "--source", "0",
                "--output", str(RESULT_PATH),
            ]
            result = _solve_via_subprocess(cmd, "mpi")
        elif runner == "fake":
            cmd = [
                "python3", str(FAKE_RUNNER), str(matrix_path),
                "--source", "0",
                "--output", str(RESULT_PATH),
            ]
            result = _solve_via_subprocess(cmd, "fake")
        else:
            result = _solve_in_process(matrix_path, source=0)
    except Exception as e:
        return jsonify({"error": str(e), "runner_attempted": runner}), 500

    return jsonify(result)


@app.route("/api/benchmark", methods=["POST"])
def benchmark() -> Any:
    data = request.get_json(force=True)
    filename = data.get("filename")
    np_values = data.get("np_values") or [1, 2, 4, 8]
    requested_runner = data.get("runner") or "mpi"

    if not filename:
        return jsonify({"error": "filename required"}), 400

    matrix_path = GRAPHS_DIR / filename
    if not matrix_path.exists():
        return jsonify({"error": f"{filename} not found"}), 404

    runner = _select_runner(requested_runner)
    rows = []
    for p in np_values:
        try:
            if runner == "mpi":
                cmd = ["mpirun", "-np", str(p), str(MPI_BINARY),
                       str(matrix_path), "--source", "0", "--output", str(RESULT_PATH)]
                r = _solve_via_subprocess(cmd, "mpi")
            elif runner == "fake":
                cmd = ["python3", str(FAKE_RUNNER), str(matrix_path),
                       "--source", "0", "--output", str(RESULT_PATH)]
                r = _solve_via_subprocess(cmd, "fake")
            else:
                r = _solve_in_process(matrix_path, source=0)
                r["num_processes"] = p  # not actually parallel but we record the slot
            rows.append({
                "np": p,
                "elapsed_seconds": r.get("elapsed_seconds"),
                "compute_seconds": r.get("compute_seconds"),
                "io_seconds": r.get("io_seconds"),
            })
        except Exception as e:
            rows.append({"np": p, "error": str(e)})

    return jsonify({"runner": runner, "results": rows})


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=True)