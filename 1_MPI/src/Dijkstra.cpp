#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <queue>

struct DijkstraResult {
    std::vector<int> l;
    std::vector<int> parent;
};

void ImportGraph(const std::string&, std::vector<std::vector<std::pair<int, int>>>&);
DijkstraResult DijkstraSingleSourceSP(const std::vector<std::vector<std::pair<int, int>>>&, int);
void PrintPath(const std::vector<int>&, int);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        std::cerr << "Too litle arguments, usage: ./Dijkstra <pathToGraphFile>" << std::endl;
        return 1;
    }

    std::vector<std::vector<std::pair<int, int>>> E;
    ImportGraph(argv[1], E);

    std::cout << "\nEdges (from, to) -> weight:\n";
    for (int i = 0; i < E.size(); i++)
    {
        for (const auto& [v, w] : E[i]) {
            std::cout << "(" << i << ", " << v << ") -> " << w << "\n";
        }
    }

    auto result = DijkstraSingleSourceSP(E, 0);

    std::cout << "\nShortest paths:\n";
    for (int i = 0; i < result.l.size(); i++)
    {
        std::cout << "vert " << i << " shortest path: ";
        if (result.l[i] == INT_MAX)
        {
            std::cout << "INF\n";
        }
        else
        {
            std::cout << result.l[i] << " | path: ";
            PrintPath(result.parent, i);
            std::cout << "\n";
        }
    }

    std::cout << "\nShortest-path tree edges:\n";
    for (int v = 0; v < result.parent.size(); v++)
    {
        if (result.parent[v] != -1)
            std::cout << result.parent[v] << " -> " << v << "\n";
    }

    return 0;
}

void ImportGraph(const std::string& filePath, std::vector<std::vector<std::pair<int, int>>>& E)
{
    std::cout << "Importing graph from file: " << filePath << std::endl; 
    std::ifstream file(filePath);
    if (!file)
    {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    std::string line;
    for (int i = 0; std::getline(file, line); i++)
    {
        E.emplace_back();
        std::stringstream lineStream(line);
        std::string valStr;
        for (int j = 0; std::getline(lineStream, valStr, '\t'); j++)
        {
            int val = std::stoi(valStr);
            if (val > 0)
                E[i].emplace_back(j, val);

            std::cout << val << "\t";
        }
        std::cout << std::endl;
    }
}


DijkstraResult DijkstraSingleSourceSP(const std::vector<std::vector<std::pair<int, int>>>& E, int s)
{
    int n = E.size();
    std::vector<int> l(n, INT_MAX);
    std::vector<int> parent(n, -1);

    using P = std::pair<int,int>; // {dist, vertex}
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    l[s] = 0;
    pq.push({l[s], s});

    while (!pq.empty())
    {
        auto [d, u] = pq.top();
        pq.pop();

        if (d != l[u])
            continue;

        for (auto [v, w] : E[u])
        {
            if (l[u] != INT_MAX && l[u] + w < l[v])
            {
                l[v] = l[u] + w;
                parent[v] = u;
                pq.push({l[v], v});
            }
        }
    }

    return DijkstraResult{l, parent};  
}

void PrintPath(const std::vector<int>& parent, int v)
{
    if (v == -1) return;
    PrintPath(parent, parent[v]);
    std::cout << v << " ";
}