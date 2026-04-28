# Parallel and Distributed Systems / Programming (PRiR) - 2026

Repository containing projects developed for the **Parallel and Distributed Systems / Programming** course in the Applied Computer Science master's program.

## 👥 Team
* Jakub Szydełko - **kubarroo**
* Jakub Urbański - **ur8an5ky**

---

## 📂 Repository structure

The repository follows a monorepo layout, split into two main assignments and a set of helper tools:

* `1_MPI/` - Project 1: parallel C/C++ application using MPI.
* `2_Distributed/` - Project 2: distributed application (Java RMI / CORBA / PGAS).
* `scripts/` - Helper Python scripts: random connected-graph generator, visualization utilities, fake_runner for testing.
* `webapp/` - Flask web application for graphical visualization of project results.

---

## 🚀 Project 1: Parallel MPI application

* **Topic:** All shortest-path trees - Dijkstra's algorithm.
* **Reference:** A. Grama, A. Gupta, *Introduction to Parallel Computing* (sec. 10.3).
* **Technologies:** C++17, MPICH 5.0.

### Brief description

The goal of the project is a parallel implementation of Dijkstra's algorithm computing shortest-path trees for every source vertex in a weighted graph (All-Pairs Shortest Path). The implementation follows the **source-parallel** formulation described in chapter 10.3 of Grama/Gupta - a single Dijkstra run is parallelized by partitioning the vertex set across MPI processes.

The number of MPI processes is dynamically matched to the size of the input adjacency matrix.

### How to run

#### Requirements

* C++ compiler with C++17 support.
* MPICH 5.0 (available in the lab via `/opt/nfs/config/source_mpich500.sh`).
* GNU Make.

#### Step 1 - connecting to a cluster node

Two-step login: first to the access server `taurus`, then to a chosen workstation from the list (for us: `stud204-00` ... `stud204-15`):

```bash
ssh user@taurus.fis.agh.edu.pl
ssh stud204-XX
```

#### Step 2 - MPI environment setup

```bash
source /opt/nfs/config/source_mpich500.sh
```

#### Step 3 - build and run

```bash
cd 1_MPI/
make            # build
make run        # run with default data (data/matrix_4.txt, NP=16)
```

The number of processes and the input file can be overridden:

```bash
make run NP=8
make run NP=16 DATA_FILE=data/matrix_100.txt
```

Cleanup:

```bash
make clean
```

### Data format

**Input** (TSV) - square adjacency matrix, tab-separated values:
* `-1` - no edge (in particular on the diagonal),
* positive integer - edge weight.

**Output**:
* Standard output - all shortest paths for every source in a readable format `s -> v: dist | path: ...`.
* JSON file - generated automatically next to the matrix file (e.g. `data/matrix_4.txt` → `data/matrix_4.json`), containing the complete set of shortest-path trees (`dist[s][v]`, `parent[s][v]`) along with timing statistics (I/O, compute, total). Used by the web application.

### Generating test matrices

```bash
cd scripts/
python3 random_matrix_generator.py 25                     # 25 vertices, density 0.4
python3 random_matrix_generator.py 100 --density 0.3      # 100 vertices, density 0.3
python3 random_matrix_generator.py 50 --seed 42           # reproducible RNG
```

The generator guarantees graph connectivity - first it builds a random spanning tree, then adds extra edges until the requested density is reached. Output is written to `1_MPI/data/matrix_<N>.txt`.

### Full documentation

A detailed description of the algorithm, parallelization (source-parallel from Grama/Gupta), flowcharts and implementation is provided in the PDF file `1_MPI/dokumentacja.pdf` (in Polish).

---

## 🌐 Project 2: Distributed application

* *(Details to be filled in at a later date)*.

---

## 🎨 Web application - results visualization

A companion Flask app providing an interactive visualization of the shortest-path trees produced by the MPI program. Lets you load any matrix, run the MPI solver or a `scipy` fallback, switch the source vertex without recomputing, and inspect individual paths by clicking nodes.

Full launch instructions (Conda or `venv`, MPI setup, port tunneling) - in [`webapp/README.md`](webapp/README.md).

---

## 🛠️ Helper tools

In the `scripts/` directory:

* `random_matrix_generator.py` - random connected weighted-graph matrix generator.
* `visualize_graph.py` - simple matplotlib graph visualization (matrix sanity-check).
* `fake_runner.py` - alternative Python solver returning results in a format compatible with `DijkstraMPI` (mainly for web app testing when the MPI binary is not yet built).