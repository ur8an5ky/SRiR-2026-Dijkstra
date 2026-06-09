#include <upcxx/upcxx.hpp>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct DistVertex
{
    int dist;
    int vertex;
};

DistVertex minloc(const DistVertex& a, const DistVertex& b)
{
    if (a.dist < b.dist) return a;
    if (b.dist < a.dist) return b;
    return (a.vertex <= b.vertex) ? a : b;
}

void ImportGraphMatrix(const std::string& path, std::vector<int>& W, int& n)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Could not open file: " + path);

    std::vector<std::vector<int>> rows;
    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string tok;
        std::vector<int> row;
        while (std::getline(ss, tok, '\t'))
        {
            row.push_back(std::stoi(tok));
        }
        rows.push_back(row);
    }

    n = static_cast<int>(rows.size());
    W.assign(n * n, 0);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            W[i * n + j] = rows[i][j];
        }
    }
}

void PrintPath(const std::vector<int>& parent, int v)
{
    if (v == -1) return;
    PrintPath(parent, parent[v]);
    std::cout << v << " ";
}

void DijkstraUPCXX(const std::vector<int>& W, int n, int s, int rank, int size, std::vector<int>& outL, std::vector<int>& outParent)
{
    int start = rank * n / size;
    int end   = (rank + 1) * n / size;

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
        DistVertex localMin{INT_MAX, -1};
        for (int v = start; v < end; v++)
        {
            if (!visited[v] && l[v] < localMin.dist)
            {
                localMin.dist = l[v];
                localMin.vertex = v;
            }
        }

        DistVertex globalMin =
            upcxx::reduce_all(localMin, minloc).wait();

        int u = globalMin.vertex;
        int distU = globalMin.dist;
        if (u == -1 || distU == INT_MAX) break;

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

    outL.assign(n, INT_MAX);
    outParent.assign(n, -1);

    for (int v = 0; v < n; v++)
    {
        int contribL = (v >= start && v < end) ? l[v] : INT_MAX;
        outL[v] = upcxx::reduce_all(contribL, [](int a, int b){ return a < b ? a : b; }).wait();

        int contribP = (v >= start && v < end) ? parent[v] : -1;
        outParent[v] = upcxx::reduce_all(contribP, [](int a, int b){ return a > b ? a : b; }).wait();
    }
    outParent[s] = -1;
}

int main(int argc, char* argv[])
{
    upcxx::init();

    int rank = upcxx::rank_me();
    int size = upcxx::rank_n();

    if (argc <= 1)
    {
        if (rank == 0)
        {
            std::cerr << "Usage: upcxx-run -n <N> ./DijkstraUPCXX <matrix>\n";
        }
        upcxx::finalize();
        return 1;
    }

    std::vector<int> W;
    int n = 0;

    if (rank == 0)
    {
        try
        {
            ImportGraphMatrix(argv[1], W, n);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to load matrix: " << e.what() << "\n";
            n = -1;
        }
    }

    n = upcxx::broadcast(n, 0).wait();
    if (n <= 0)
    {
        upcxx::finalize();
        return 1;
    }

    if (rank != 0)
    {
        W.resize(n * n);
    }

    upcxx::broadcast(W.data(), n * n, 0).wait();

    for (int s = 0; s < n; s++)
    {
        std::vector<int> l, parent;
        DijkstraUPCXX(W, n, s, rank, size, l, parent);

        if (rank == 0)
        {
            std::cout << "\nShortest paths from source " << s << ":\n";
            for (int v = 0; v < n; v++)
            {
                std::cout << s << " -> " << v << ": ";
                if (l[v] == INT_MAX)
                {
                    std::cout << "INF\n";
                }
                else
                {
                    std::cout << l[v] << " | path: ";
                    PrintPath(parent, v);
                    std::cout << "\n";
                }
            }
        }
    }

    upcxx::finalize();
    return 0;
}