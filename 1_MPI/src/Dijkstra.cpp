#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <queue>

void ImportGraph(const std::string&, std::vector<std::vector<std::pair<int, int>>>&);
std::vector<int> DijkstraSingleSourceSP(const std::vector<std::vector<std::pair<int, int>>>&, int);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        std::cerr << "Too litle arguments, usage: ./Dijkstra <pathToGraphFile>" << std::endl;
        return 1;
    }

    std::vector<std::vector<std::pair<int, int>>> E;
    ImportGraph(argv[1], E);

    std::cout << "Edges (from, to) -> weight:\n";
    for (int i = 0; i < E.size(); i++)
    {
        for (const auto& [v, w] : E[i]) {
            std::cout << "(" << i << ", " << v << ") -> " << w << "\n";
        }
    }

    auto l = DijkstraSingleSourceSP(E, 0);

    std::cout << "Shortest paths:\n";
    for (int i = 0; i < l.size(); i++)
    {
        std::cout << "vert " << i << " shortest path: ";
        if (l[i] == INT_MAX)
            std::cout << "INF" << std::endl;
        else
            std::cout << l[i] << std::endl;
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


std::vector<int> DijkstraSingleSourceSP(const std::vector<std::vector<std::pair<int, int>>>& E, int s)
{
    int n = E.size();
    std::vector<int> l(n, INT_MAX);

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
                pq.push({l[v], v});
            }
        }
    }

    return l;  
}