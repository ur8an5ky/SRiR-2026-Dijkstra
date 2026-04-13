#include <mpi.h>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int INF = INT_MAX / 4;

int BlockLow(int id, int p, int n) {
    return (id * n) / p;
}

int BlockHigh(int id, int p, int n) {
    return BlockLow(id + 1, p, n) - 1;
}

int BlockSize(int id, int p, int n) {
    return BlockHigh(id, p, n) - BlockLow(id, p, n) + 1;
}

int Owner(int v, int p, int n) {
    return ((v + 1) * p - 1) / n;
}

std::vector<std::vector<int>> ImportMatrix(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    std::vector<std::vector<int>> W;
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        std::vector<int> row;

        while (std::getline(ss, token, '\t')) {
            int val = std::stoi(token);
            if (val <= 0)
                row.push_back(INF);
            else
                row.push_back(val);
        }
        W.push_back(row);
    }

    for (int i = 0; i < (int)W.size(); i++) {
        if (i < (int)W[i].size()) {
            W[i][i] = 0;
        }
    }

    return W;
}

void PrintPath(const std::vector<int>& parent, int v) {
    if (v == -1) return;
    PrintPath(parent, parent[v]);
    std::cout << v << " ";
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc <= 1) {
        if (rank == 0) {
            std::cerr << "Usage: mpirun -np <p> ./DijkstraMPI <graphFile>\n";
        }
        MPI_Finalize();
        return 1;
    }

    int n = 0;
    std::vector<std::vector<int>> fullW;

    if (rank == 0) {
        fullW = ImportMatrix(argv[1]);
        n = (int)fullW.size();
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int localStart = BlockLow(rank, size, n);
    int localEnd   = BlockHigh(rank, size, n);
    int localN     = BlockSize(rank, size, n);

    std::vector<int> localW(localN * n);

    // Przygotowanie danych do Scatterv
    std::vector<int> sendcounts, displs;
    if (rank == 0) {
        sendcounts.resize(size);
        displs.resize(size);

        for (int p = 0; p < size; p++) {
            int rows = BlockSize(p, size, n);
            sendcounts[p] = rows * n;
            displs[p] = BlockLow(p, size, n) * n;
        }
    }

    std::vector<int> flatW;
    if (rank == 0) {
        flatW.resize(n * n);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                flatW[i * n + j] = fullW[i][j];
            }
        }
    }

    MPI_Scatterv(rank == 0 ? flatW.data() : nullptr,
                 rank == 0 ? sendcounts.data() : nullptr,
                 rank == 0 ? displs.data() : nullptr,
                 MPI_INT,
                 localW.data(),
                 localN * n,
                 MPI_INT,
                 0,
                 MPI_COMM_WORLD);

    int source = 0;

    std::vector<int> dist(n, INF);
    std::vector<int> parent(n, -1);
    std::vector<int> visited(n, 0);

    dist[source] = 0;

    for (int iter = 0; iter < n; iter++) {
        int localMinDist = INF;
        int localMinVertex = -1;

        // Lokalny wybór minimum tylko z własnego bloku
        for (int v = localStart; v <= localEnd; v++) {
            if (!visited[v] && dist[v] < localMinDist) {
                localMinDist = dist[v];
                localMinVertex = v;
            }
        }

        struct {
            int value;
            int index;
        } localPair, globalPair;

        localPair.value = localMinDist;
        localPair.index = (localMinVertex == -1 ? -1 : localMinVertex);

        // Uwaga: dla MPI_MINLOC lepiej dać INF, gdy brak kandydata
        if (localPair.index == -1) {
            localPair.value = INF;
            localPair.index = -1;
        }

        MPI_Allreduce(&localPair, &globalPair, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

        int u = globalPair.index;
        int uDist = globalPair.value;

        if (u == -1 || uDist >= INF) {
            break; // brak osiągalnych dalszych wierzchołków
        }

        visited[u] = 1;

        // Każdy proces aktualizuje dystanse dla swoich lokalnych wierzchołków
        for (int v = localStart; v <= localEnd; v++) {
            if (!visited[v]) {
                int w = localW[(v - localStart) * n + u]; // to byłoby dla kolumny u w lokalnych wierszach
            }
        }

        // Potrzebujemy wiersza u, bo relaksujemy krawędzie u -> v
        std::vector<int> rowU(n, INF);

        int owner = Owner(u, size, n);
        if (rank == owner) {
            int localRow = u - localStart;
            for (int j = 0; j < n; j++) {
                rowU[j] = localW[localRow * n + j];
            }
        }

        MPI_Bcast(rowU.data(), n, MPI_INT, owner, MPI_COMM_WORLD);

        for (int v = localStart; v <= localEnd; v++) {
            if (!visited[v] && rowU[v] < INF && dist[u] + rowU[v] < dist[v]) {
                dist[v] = dist[u] + rowU[v];
                parent[v] = u;
            }
        }

        // Uzgadniamy globalny dist i parent
        std::vector<int> newDist(n, INF);
        std::vector<int> newParent(n, -1);

        for (int v = localStart; v <= localEnd; v++) {
            newDist[v] = dist[v];
            newParent[v] = parent[v];
        }

        MPI_Allreduce(newDist.data(), dist.data(), n, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

        // Parent nie da się sensownie zredukować przez MIN.
        // Najprościej zebrać parent od właścicieli:
        for (int v = 0; v < n; v++) {
            int candidate = -1;
            if (v >= localStart && v <= localEnd) {
                candidate = parent[v];
            }
            MPI_Allreduce(&candidate, &parent[v], 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        std::cout << "\nShortest paths from source " << source << ":\n";
        for (int v = 0; v < n; v++) {
            std::cout << "vert " << v << ": ";
            if (dist[v] >= INF) {
                std::cout << "INF\n";
            } else {
                std::cout << dist[v] << " | path: ";
                PrintPath(parent, v);
                std::cout << "\n";
            }
        }

        std::cout << "\nShortest-path tree edges:\n";
        for (int v = 0; v < n; v++) {
            if (parent[v] != -1) {
                std::cout << parent[v] << " -> " << v << "\n";
            }
        }
    }

    MPI_Finalize();
    return 0;
}