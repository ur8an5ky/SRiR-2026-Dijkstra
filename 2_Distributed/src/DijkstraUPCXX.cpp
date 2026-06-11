#include <upcxx/upcxx.hpp>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <stdexcept>

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
    {
        throw std::runtime_error("Could not open file: " + path);
    }

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

struct Args
{
    std::string matrixPath;
    std::string outputPath;
};

Args ParseArgs(int argc, char* argv[])
{
    Args a;
    if (argc < 2)
    {
        throw std::runtime_error("missing matrix path");
    }

    for (int i = 1; i < argc; i++)
    {
        std::string tok = argv[i];
        if (tok == "--output")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("--output requires a value");
            }
            a.outputPath = argv[++i];
        }
        else if (tok.size() > 2 && tok.substr(0, 2) == "--")
        {
            throw std::runtime_error("unknown flag: " + tok);
        }
        else
        {
            if (!a.matrixPath.empty())
            {
                throw std::runtime_error("multiple matrix paths given");
            }
            a.matrixPath = tok;
        }
    }
    if (a.matrixPath.empty())
    {
        throw std::runtime_error("matrix path is required");
    }

    return a;
}

std::string ResolveOutputPath(const std::string& outputArg, const std::string& matrixPath)
{
    namespace fs = std::filesystem;
    fs::path matrix(matrixPath);

    if (outputArg.empty())
    {
        fs::path out = matrix;
        out.replace_extension(".json");

        return out.string();
    }
    fs::path out(outputArg);
    if (out.extension() == ".json")
    {
        return out.string();
    }
    fs::create_directories(out);
    
    return (out / (matrix.stem().string() + ".json")).string();
}

void WriteJsonResult(const std::string& path, int n, int np, double elapsed, double ioTime, double computeTime,
                     const std::vector<std::vector<int>>& allDist, const std::vector<std::vector<int>>& allParent)
{
    std::ofstream out(path);
    if (!out)
    {
        throw std::runtime_error("cannot open output file: " + path);
    }

    out << "{\n";
    out << "  \"n\": " << n << ",\n";
    out << "  \"num_processes\": " << np << ",\n";
    out << "  \"io_seconds\": " << ioTime << ",\n";
    out << "  \"compute_seconds\": " << computeTime << ",\n";
    out << "  \"elapsed_seconds\": " << elapsed << ",\n";

    out << "  \"dist\": [\n";
    for (int s = 0; s < n; s++)
    {
        out << "    [";
        for (int v = 0; v < n; v++)
        {
            out << (allDist[s][v] == INT_MAX ? -1 : allDist[s][v]);
            if (v < n - 1) out << ", ";
        }
        out << "]";
        if (s < n - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    out << "  \"parent\": [\n";
    for (int s = 0; s < n; s++)
    {
        out << "    [";
        for (int v = 0; v < n; v++)
        {
            out << allParent[s][v];
            if (v < n - 1) out << ", ";
        }
        out << "]";
        if (s < n - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
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

    Args args;
    try
    {
        args = ParseArgs(argc, argv);
    }
    catch (const std::exception& e)
    {
        if (rank == 0)
        {
            std::cerr << "Error: " << e.what() << "\n\n"
                      << "Usage:\n"
                      << "  upcxx-run -n N ./DijkstraUPCXX <matrix> [--output PATH]\n";
        }
        upcxx::finalize();
        return 1;
    }

    upcxx::barrier();
    auto tIoStart = std::chrono::steady_clock::now();

    std::vector<int> W;
    int n = 0;

    if (rank == 0)
    {
        try
        {
            ImportGraphMatrix(args.matrixPath, W, n);
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
        W.resize(n * n);
    upcxx::broadcast(W.data(), n * n, 0).wait();

    upcxx::barrier();
    double ioTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - tIoStart).count();

    upcxx::barrier();
    auto tComputeStart = std::chrono::steady_clock::now();

    std::vector<std::vector<int>> allDist;
    std::vector<std::vector<int>> allParent;
    if (rank == 0)
    {
        allDist.reserve(n);
        allParent.reserve(n);
    }

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
            allDist.push_back(std::move(l));
            allParent.push_back(std::move(parent));
        }
    }

    upcxx::barrier();
    double computeTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - tComputeStart).count();

    if (rank == 0)
    {
        std::string outPath = ResolveOutputPath(args.outputPath, args.matrixPath);
        double elapsed = ioTime + computeTime;
        WriteJsonResult(outPath, n, size, elapsed, ioTime, computeTime, allDist, allParent);
        
        std::cerr << "\n[JSON] wrote " << outPath
                  << " (n=" << n
                  << ", np=" << size
                  << ", io=" << ioTime << "s"
                  << ", compute=" << computeTime << "s"
                  << ", total=" << elapsed << "s)\n";
    }

    upcxx::finalize();

    return 0;
}