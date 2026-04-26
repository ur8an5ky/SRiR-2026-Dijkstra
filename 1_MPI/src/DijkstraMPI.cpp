#include <mpi.h>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct DijkstraResult {
    std::vector<int> l;
    std::vector<int> parent;
};

void ImportGraphMatrix(const std::string& filePath, std::vector<int>& W, int& n);
DijkstraResult DijkstraMPI(const std::vector<int>& W, int n, int s, int rank, int size);
void PrintPath(const std::vector<int>& parent, int v);

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc <= 1)
    {
        if (rank == 0)
            std::cerr << "Usage: mpiexec -f <fileWithNodes> -n <numberOfProcesses> ./DijkstraMPI <pathToGraphFile>\n";

        MPI_Finalize();
        return 1;
    }

    std::vector<int> W;
    int n = 0;

    if (rank == 0)
        ImportGraphMatrix(argv[1], W, n);

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
        W.resize(n * n);

    MPI_Bcast(W.data(), n * n, MPI_INT, 0, MPI_COMM_WORLD);

    for (int s = 0; s < n; s++)
    {
        auto result = DijkstraMPI(W, n, s, rank, size);

        if (rank == 0)
        {
            std::cout << "\nShortest paths from source " << s << ":\n";

            for (int v = 0; v < n; v++)
            {
                std::cout << s << " -> " << v << ": ";

                if (result.l[v] == INT_MAX)
                {
                    std::cout << "INF\n";
                }
                else
                {
                    std::cout << result.l[v] << " | path: ";
                    PrintPath(result.parent, v);
                    std::cout << "\n";
                }
            }
        }
    }

    MPI_Finalize();
    return 0;
}

void ImportGraphMatrix(const std::string& filePath, std::vector<int>& W, int& n)
{
    std::ifstream file(filePath);

    if (!file)
        throw std::runtime_error("Could not open file: " + filePath);

    std::vector<std::vector<int>> rows;
    std::string line;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string valStr;
        std::vector<int> row;

        while (std::getline(ss, valStr, '\t'))
            row.push_back(std::stoi(valStr));

        rows.push_back(row);
    }

    n = rows.size();
    W.assign(n * n, 0);

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            W[i * n + j] = rows[i][j];
}

DijkstraResult DijkstraMPI(const std::vector<int>& W, int n, int s, int rank, int size)
{
    int start = rank * n / size;
    int end = (rank + 1) * n / size;

    std::vector<int> l(n, INT_MAX);
    std::vector<int> parent(n, -1);
    std::vector<int> visited(n, 0);

    visited[s] = 1;

    for (int v = start; v < end; v++)
    {
        if (v == s)
        {
            l[v] = 0;
        }
        else if (W[s * n + v] > 0)
        {
            l[v] = W[s * n + v];
            parent[v] = s;
        }
    }

    for (int step = 1; step < n; step++)
    {
        struct {
            int dist;
            int vertex;
        } localMin, globalMin;

        localMin.dist = INT_MAX;
        localMin.vertex = -1;

        for (int v = start; v < end; v++)
        {
            if (!visited[v] && l[v] < localMin.dist)
            {
                localMin.dist = l[v];
                localMin.vertex = v;
            }
        }

        MPI_Allreduce(
            &localMin,
            &globalMin,
            1,
            MPI_2INT,
            MPI_MINLOC,
            MPI_COMM_WORLD
        );

        int u = globalMin.vertex;
        int distU = globalMin.dist;

        if (u == -1 || distU == INT_MAX)
            break;

        visited[u] = 1;

        for (int v = start; v < end; v++)
        {
            int w = W[u * n + v];

            if (!visited[v] && w > 0 && distU + w < l[v])
            {
                l[v] = distU + w;
                parent[v] = u;
            }
        }
    }

    std::vector<int> counts(size);
    std::vector<int> displs(size);

    for (int p = 0; p < size; p++)
    {
        int pStart = p * n / size;
        int pEnd = (p + 1) * n / size;

        counts[p] = pEnd - pStart;
        displs[p] = pStart;
    }

    std::vector<int> finalL(n);
    std::vector<int> finalParent(n);

    MPI_Gatherv(
        l.data() + start,
        end - start,
        MPI_INT,
        finalL.data(),
        counts.data(),
        displs.data(),
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    MPI_Gatherv(
        parent.data() + start,
        end - start,
        MPI_INT,
        finalParent.data(),
        counts.data(),
        displs.data(),
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    return DijkstraResult{finalL, finalParent};
}

void PrintPath(const std::vector<int>& parent, int v)
{
    if (v == -1) return;

    PrintPath(parent, parent[v]);
    std::cout << v << " ";
}